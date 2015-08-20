#include "resampler.h"

#include <math.h>
#include <stdlib.h>

/* Copyright (C) 2004-2008 Shay Green.
   Copyright (C) 2015 Christopher Snowhill. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#undef PI
#define PI 3.1415926535897932384626433832795029

typedef int16_t imp_t;

#if RESAMPLER_BITS == 16
typedef int32_t intermediate_t;
#elif RESAMPLER_BITS == 32
typedef int64_t intermediate_t;
#endif

static void gen_sinc( double rolloff, int width, double offset, double spacing, double scale,
		int count, imp_t* out )
{
	double const maxh = 256;
	double const step = PI / maxh * spacing;
	double const to_w = maxh * 2 / width;
	double const pow_a_n = pow( rolloff, maxh );
	scale /= maxh * 2;
	double angle = (count / 2 - 1 + offset) * -step;

	while ( count-- )
	{
		*out++ = 0;
		double w = angle * to_w;
		if ( fabs( w ) < PI )
		{
			double rolloff_cos_a = rolloff * cos( angle );
			double num = 1 - rolloff_cos_a -
					pow_a_n * cos( maxh * angle ) +
					pow_a_n * rolloff * cos( (maxh - 1) * angle );
			double den = 1 - rolloff_cos_a - rolloff_cos_a + rolloff * rolloff;
			double sinc = scale * num / den - scale;

			out [-1] = (imp_t) (cos( w ) * sinc + sinc);
		}
		angle += step;
	}
}

enum { width = 16 };
enum { stereo = 2 };
enum { max_res = 32 };
enum { min_width = (width < 4 ? 4 : width) };
enum { adj_width = min_width / 4 * 4 + 2 };
enum { write_offset = adj_width * stereo };

enum { buffer_size = 32 };

typedef struct _resampler
{
	int width_;
	int rate_;
	int inptr;
	int infilled;
	int outptr;
	int outfilled;

	imp_t const* imp;
	imp_t impulses [max_res * (adj_width + 2)];
	sample_t buffer_in[buffer_size * stereo * 2];
	sample_t buffer_out[buffer_size * stereo];
} resampler;

void * resampler_create()
{
	resampler *r = (resampler *) malloc(sizeof(resampler));
	if (r) resampler_clear(r);
	return r;
}

void resampler_destroy(void *r)
{
	free(r);
}

void resampler_clear(void *_r)
{
	resampler * r = (resampler *)_r;
	r->width_ = adj_width;
	r->inptr = 0;
	r->infilled = 0;
	r->outptr = 0;
	r->outfilled = 0;
	r->imp = r->impulses;

	resampler_set_rate(r, 1.0);
}

void resampler_set_rate( void *_r, double new_factor )
{
	resampler *rs = (resampler *)_r;

	double const rolloff = 0.999;
	double const gain = 1.0;

	// determine number of sub-phases that yield lowest error
	double ratio_ = 0.0;
	int res = -1;
	{
		double least_error = 2;
		double pos = 0;
		for ( int r = 1; r <= max_res; r++ )
		{
			pos += new_factor;
			double nearest = floor( pos + 0.5 );
			double error = fabs( pos - nearest );
			if ( error < least_error )
			{
				res = r;
				ratio_ = nearest / res;
				least_error = error;
			}
		}
	}
	rs->rate_ = ratio_;

	// how much of input is used for each output sample
	int const step = stereo * (int) floor( ratio_ );
	double fraction = fmod( ratio_, 1.0 );

	double const filter = (ratio_ < 1.0) ? 1.0 : 1.0 / ratio_;
	double pos = 0.0;
	//int input_per_cycle = 0;
	imp_t* out = rs->impulses;
	for ( int n = res; --n >= 0; )
	{
		gen_sinc( rolloff, (int) (rs->width_ * filter + 1) & ~1, pos, filter,
				(double) (0x7FFF * gain * filter), (int) rs->width_, out );
		out += rs->width_;

		int cur_step = step;
		pos += fraction;
		if ( pos >= 0.9999999 )
		{
			pos -= 1.0;
			cur_step += stereo;
		}

		*out++ = (cur_step - rs->width_ * 2 + 4) * sizeof (sample_t);
		*out++ = 4 * sizeof (imp_t);
		//input_per_cycle += cur_step;
	}
	// last offset moves back to beginning of impulses
	out [-1] -= (char*) out - (char*) rs->impulses;

	rs->imp = rs->impulses;
}

int resampler_get_free(void *_r)
{
	resampler *r = (resampler *)_r;
	return buffer_size * stereo - r->infilled;
}

int resampler_get_min_fill(void *_r)
{
	resampler *r = (resampler *)_r;
	int total_free = buffer_size * stereo - r->infilled;
	const int min_needed = write_offset + stereo;
	return total_free >= min_needed ? min_needed : total_free;
}

void resampler_write_pair(void *_r, sample_t ls, sample_t rs)
{
	resampler *r = (resampler *)_r;

	if (r->infilled < buffer_size * stereo)
	{
		r->buffer_in[r->inptr + 0] = ls;
		r->buffer_in[r->inptr + 1] = rs;
		r->buffer_in[buffer_size * stereo + r->inptr + 0] = ls;
		r->buffer_in[buffer_size * stereo + r->inptr + 1] = rs;
		r->inptr = (r->inptr + stereo) % (buffer_size * stereo);
		r->infilled += stereo;
	}
}

static const sample_t * resampler_inner_loop( resampler *r, sample_t** out_,
		sample_t const* out_end, sample_t const in [], int in_size )
{
	in_size -= write_offset;
	if ( in_size > 0 )
	{
		sample_t* restrict out = *out_;
		sample_t const* const in_end = in + in_size;
		imp_t const* imp = r->imp;

		do
		{
			// accumulate in extended precision
			int pt = imp [0];
			intermediate_t l = pt * in [0];
			intermediate_t r = pt * in [1];
			if ( out >= out_end )
				break;
			for ( int n = (adj_width - 2) / 2; n; --n )
			{
				pt = imp [1];
				l += pt * in [2];
				r += pt * in [3];

				// pre-increment more efficient on some RISC processors
				imp += 2;
				pt = imp [0];
				r += pt * in [5];
				in += 4;
				l += pt * in [0];
			}
			pt = imp [1];
			l += pt * in [2];
			r += pt * in [3];

			// these two "samples" after the end of the impulse give the
			// proper offsets to the next input sample and next impulse
			in  = (sample_t const*) ((char const*) in  + imp [2]); // some negative value
			imp = (imp_t const*) ((char const*) imp + imp [3]); // small positive or large negative

			out [0] = (sample_t) (l >> 15);
			out [1] = (sample_t) (r >> 15);
			out += 2;
		}
		while ( in < in_end );

		r->imp = imp;
		*out_ = out;
	}
	return in;
}

static int resampler_wrapper( resampler *r, sample_t out [], int* out_size,
		sample_t const in [], int in_size )
{
	sample_t* out_ = out;
	int result = resampler_inner_loop( r, &out_, out + *out_size, in, in_size ) - in;

	*out_size = out_ - out;
	return result;
}

static void resampler_fill( resampler *r )
{
	while (!r->outfilled && r->infilled)
	{
		int writepos = ( r->outptr + r->outfilled ) % (buffer_size * stereo);
		int writesize = (buffer_size * stereo) - writepos;
		if ( writesize > ( buffer_size * stereo - r->outfilled ) )
				writesize = buffer_size * stereo - r->outfilled;
		int inread = resampler_wrapper(r, &r->buffer_out[writepos], &writesize, &r->buffer_in[buffer_size * stereo + r->inptr - r->infilled], r->infilled);
		r->infilled -= inread;
		r->outfilled += writesize;
		if (!inread)
			break;
	}
}

int resampler_get_avail(void *_r)
{
	resampler *r = (resampler *)_r;
	if (r->outfilled < stereo && r->infilled >= r->width_)
	  resampler_fill( r );
	return r->outfilled;
}

void resampler_read_pair( void *_r, sample_t *ls, sample_t *rs )
{
	resampler *r = (resampler *)_r;

	if (r->outfilled < stereo)
	  resampler_fill( r );
	if (r->outfilled < stereo)
	{
		*ls = 0;
		*rs = 0;
		return;
	}
	*ls = r->buffer_out[r->outptr + 0];
	*rs = r->buffer_out[r->outptr + 1];
	r->outptr = (r->outptr + 2) % (buffer_size * stereo);
	r->outfilled -= stereo;
}
