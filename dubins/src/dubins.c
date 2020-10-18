// Copyright (c) 2008-2014, Andrew Walker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _USE_MATH_DEFINES

#include "dubins.h"
#include <math.h>
#include <assert.h>

#ifdef _MSC_VER
#define INFINITY HUGE_VAL
static double fmin(double a, double b) { return (a < b) ? a : b; }
#endif

#define EPSILON (10e-10)

#define LSL (0)
#define LSR (1)
#define RSL (2)
#define RSR (3)
#define RLR (4)
#define LRL (5)

// The three segment types a path can be made up of
#define L_SEG (0)
#define S_SEG (1)
#define R_SEG (2)

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#ifndef INFINITY
    #define INFINITY HUGE_VAL
#endif

// The segment types for each of the Path types
const int DIRDATA[][3] = {
    { L_SEG, S_SEG, L_SEG },
    { L_SEG, S_SEG, R_SEG },
    { R_SEG, S_SEG, L_SEG },
    { R_SEG, S_SEG, R_SEG },
    { R_SEG, L_SEG, R_SEG },
    { L_SEG, R_SEG, L_SEG }
};

DubinsWord dubins_words[] = {
    dubins_LSL,
    dubins_LSR,
    dubins_RSL,
    dubins_RSR,
    dubins_RLR,
    dubins_LRL,
};

#define UNPACK_INPUTS(alpha, beta)     \
    double sa = sin(alpha);            \
    double sb = sin(beta);             \
    double ca = cos(alpha);            \
    double cb = cos(beta);             \
    double c_ab = cos(alpha - beta);   \

#define PACK_OUTPUTS(outputs)       \
    outputs[0]  = t;                \
    outputs[1]  = p;                \
    outputs[2]  = q;

/**
 * Floating point modulus suitable for rings
 *
 * fmod doesn't behave correctly for angular quantities, this function does
 */
double fmodr( double x, double y)
{
    return x - y*floor(x/y);
}

double mod2pi( double theta )
{
    return fmodr( theta, 2 * M_PI );
}

int dubins_init_normalised( double alpha, double beta, double d, DubinsPath* path)
{
    double params[3];
    double best_cost;
    double cost;
    int    best_word;
    int    i;
    int    err;

    best_cost = INFINITY;
    best_word = -1;
    for( i = 0; i < 6; i++ ) {
        err = dubins_words[i](alpha, beta, d, params);
        if(err == EDUBOK) {
            cost = params[0] + params[1] + params[2];
            if(cost < best_cost) {
                best_word = i;
                best_cost = cost;
                path->param[0] = params[0];
                path->param[1] = params[1];
                path->param[2] = params[2];
                path->type = i;
            }
        }
    }

    if(best_word == -1) {
        return EDUBNOPATH;
    }
    path->type = best_word;
    return EDUBOK;
}

int dubins_init( double q0[3], double q1[3], double rho, DubinsPath* path )
{
    int i;
    double dx, dy, D, d, theta, alpha, beta;

    dx = q1[0] - q0[0];
    dy = q1[1] - q0[1];
    D = sqrt( dx * dx + dy * dy );
    d = D / rho;
    if( rho <= 0.0 ) {
        return EDUBBADRHO;
    }
    theta = mod2pi(atan2( dy, dx ));
    alpha = mod2pi(q0[2] - theta);
    beta  = mod2pi(q1[2] - theta);
    for( i = 0; i < 3; i ++ ) {
        path->qi[i] = q0[i];
    }
    path->rho = rho;

    return dubins_init_normalised( alpha, beta, d, path );
}

int dubins_LSL( double alpha, double beta, double d, double* outputs )
{
    double tmp0, p_squared, tmp1, t, p, q;
    UNPACK_INPUTS(alpha, beta);
    tmp0 = d+sa-sb;
    p_squared = 2 + (d*d) -(2*c_ab) + (2*d*(sa - sb));
    if( p_squared < 0 ) {
        return EDUBNOPATH;
    }
    tmp1 = atan2( (cb-ca), tmp0 );
    t = mod2pi(-alpha + tmp1 );
    p = sqrt( p_squared );
    q = mod2pi(beta - tmp1 );
    PACK_OUTPUTS(outputs);
    return EDUBOK;
}

int dubins_RSR( double alpha, double beta, double d, double* outputs )
{
    double tmp0, p_squared, tmp1, t, p, q;
    UNPACK_INPUTS(alpha, beta);
    tmp0 = d-sa+sb;
    p_squared = 2 + (d*d) -(2*c_ab) + (2*d*(sb-sa));
    if( p_squared < 0 ) {
        return EDUBNOPATH;
    }
    tmp1 = atan2( (ca-cb), tmp0 );
    t = mod2pi( alpha - tmp1 );
    p = sqrt( p_squared );
    q = mod2pi( -beta + tmp1 );
    PACK_OUTPUTS(outputs);
    return EDUBOK;
}

int dubins_LSR( double alpha, double beta, double d, double* outputs )
{
    double p_squared, p, tmp2, t, q;
    UNPACK_INPUTS(alpha, beta);
    p_squared = -2 + (d*d) + (2*c_ab) + (2*d*(sa+sb));
    if( p_squared < 0 ) {
        return EDUBNOPATH;
    }
    p    = sqrt( p_squared );
    tmp2 = atan2( (-ca-cb), (d+sa+sb) ) - atan2(-2.0, p);
    t    = mod2pi(-alpha + tmp2);
    q    = mod2pi( -mod2pi(beta) + tmp2 );
    PACK_OUTPUTS(outputs);
    return EDUBOK;
}

int dubins_RSL( double alpha, double beta, double d, double* outputs )
{
    double p_squared, p, tmp2, t, q;
    UNPACK_INPUTS(alpha, beta);
    p_squared = (d*d) -2 + (2*c_ab) - (2*d*(sa+sb));
    if( p_squared< 0 ) {
        return EDUBNOPATH;
    }
    p    = sqrt( p_squared );
    tmp2 = atan2( (ca+cb), (d-sa-sb) ) - atan2(2.0, p);
    t    = mod2pi(alpha - tmp2);
    q    = mod2pi(beta - tmp2);
    PACK_OUTPUTS(outputs);
    return EDUBOK;
}

int dubins_RLR( double alpha, double beta, double d, double* outputs )
{
    double tmp_rlr, p, t, q;
    UNPACK_INPUTS(alpha, beta);
    tmp_rlr = (6. - d*d + 2*c_ab + 2*d*(sa-sb)) / 8.;
    if( fabs(tmp_rlr) > 1) {
        return EDUBNOPATH;
    }
    p = mod2pi( 2*M_PI - acos( tmp_rlr ) );
    t = mod2pi(alpha - atan2( ca-cb, d-sa+sb ) + mod2pi(p/2.));
    q = mod2pi(alpha - beta - t + mod2pi(p));
    PACK_OUTPUTS( outputs );
    return EDUBOK;
}

int dubins_LRL( double alpha, double beta, double d, double* outputs )
{
    double tmp_lrl, p, t, q;
    UNPACK_INPUTS(alpha, beta);
    tmp_lrl = (6. - d*d + 2*c_ab + 2*d*(- sa + sb)) / 8.;
    if( fabs(tmp_lrl) > 1) {
        return EDUBNOPATH;
    }
    p = mod2pi( 2*M_PI - acos( tmp_lrl ) );
    t = mod2pi(-alpha - atan2( ca-cb, d+sa-sb ) + p/2.);
    q = mod2pi(mod2pi(beta) - alpha -t + mod2pi(p));
    PACK_OUTPUTS( outputs );
    return EDUBOK;
}

double dubins_path_length( DubinsPath* path )
{
    double length = 0.;
    length += path->param[0];
    length += path->param[1];
    length += path->param[2];
    length = length * path->rho;
    return length;
}

double dubins_segment_length( DubinsPath* path, int i )
{
    if( (i < 0) || (i > 2) )
    {
        return INFINITY;
    }
    return path->param[i] * path->rho;
}

double dubins_segment_length_normalized( DubinsPath* path, int i )
{
    if( (i < 0) || (i > 2) )
    {
        return INFINITY;
    }
    return path->param[i];
}

int dubins_path_type( DubinsPath* path ) {
    return path->type;
}

void dubins_segment( double t, double qi[3], double qt[3], int type)
{
    assert( type == L_SEG || type == S_SEG || type == R_SEG );

    if( type == L_SEG ) {
        qt[0] = qi[0] + sin(qi[2]+t) - sin(qi[2]);
        qt[1] = qi[1] - cos(qi[2]+t) + cos(qi[2]);
        qt[2] = qi[2] + t;
    }
    else if( type == R_SEG ) {
        qt[0] = qi[0] - sin(qi[2]-t) + sin(qi[2]);
        qt[1] = qi[1] + cos(qi[2]-t) - cos(qi[2]);
        qt[2] = qi[2] - t;
    }
    else if( type == S_SEG ) {
        qt[0] = qi[0] + cos(qi[2]) * t;
        qt[1] = qi[1] + sin(qi[2]) * t;
        qt[2] = qi[2];
    }
}

int dubins_path_sample( DubinsPath* path, double t, double q[3] )
{
    double tprime;
    double qi[3], q1[3], q2[3];
    double p1, p2;
    const int* types;

    if( t < 0 || t >= dubins_path_length(path) ) {
        // error, parameter out of bounds
        return EDUBPARAM;
    }

    // tprime is the normalised variant of the parameter t
    tprime = t / path->rho;

    // In order to take rho != 1 into account this function needs to be more complex
    // than it would be otherwise. The transformation is done in five stages.
    //
    // 1. translate the components of the initial configuration to the origin
    // 2. generate the target configuration
    // 3. transform the target configuration
    //      scale the target configuration
    //      translate the target configration back to the original starting point
    //      normalise the target configurations angular component

    // The translated initial configuration
    qi[0] = 0.0;
    qi[1] = 0.0;
    qi[2] = path->qi[2];

    // Generate the target configuration
    types = DIRDATA[path->type];
    p1 = path->param[0];
    p2 = path->param[1];
    dubins_segment( p1,      qi,    q1, types[0] );
    dubins_segment( p2,      q1,    q2, types[1] );
    if( tprime < p1 ) {
        dubins_segment( tprime, qi, q, types[0] );
    }
    else if( tprime < (p1+p2) ) {
        dubins_segment( tprime-p1, q1, q,  types[1] );
    }
    else {
        dubins_segment( tprime-p1-p2, q2, q,  types[2] );
    }

    // scale the target configuration, translate back to the original starting point
    q[0] = q[0] * path->rho + path->qi[0];
    q[1] = q[1] * path->rho + path->qi[1];
    q[2] = mod2pi(q[2]);

    return 0;
}

int dubins_path_sample_many( DubinsPath* path, DubinsPathSamplingCallback cb, double stepSize, void* user_data )
{
    double x, length, q[3];
    int retcode;

    x = 0.0;
    length = dubins_path_length(path);
    while( x <  length ) {
        q[3];
        dubins_path_sample( path, x, q );
        retcode = cb(q, x, user_data);
        if( retcode != 0 ) {
            return retcode;
        }
        x += stepSize;
    }
    return 0;
}

int dubins_path_endpoint( DubinsPath* path, double q[3] )
{
    // TODO - introduce a new constant rather than just using EPSILON
    return dubins_path_sample( path, dubins_path_length(path) - EPSILON, q );
}

int dubins_extract_subpath( DubinsPath* path, double t, DubinsPath* newpath )
{
    double tprime;

    // calculate the true parameter
    tprime = t / path->rho;

    // copy most of the data
    newpath->qi[0] = path->qi[0];
    newpath->qi[1] = path->qi[1];
    newpath->qi[2] = path->qi[2];
    newpath->rho   = path->rho;
    newpath->type  = path->type;

    // fix the parameters
    newpath->param[0] = fmin( path->param[0], tprime );
    newpath->param[1] = fmin( path->param[1], tprime - newpath->param[0]);
    newpath->param[2] = fmin( path->param[2], tprime - newpath->param[0] - newpath->param[1]);
    return 0;
}


