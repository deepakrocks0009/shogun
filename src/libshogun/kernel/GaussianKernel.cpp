/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 1999-2010 Soeren Sonnenburg
 * Written (W) 2011 Abhinav Maurya
 * Copyright (C) 1999-2009 Fraunhofer Institute FIRST and Max-Planck-Society
 * Copyright (C) 2010 Berlin Institute of Technology
 */

#include "lib/common.h"
#include "base/Parameter.h"
#include "kernel/GaussianKernel.h"
#include "features/DotFeatures.h"
#include "lib/io.h"

using namespace shogun;

CGaussianKernel::CGaussianKernel()
	: CDotKernel()
{
	init();
}

CGaussianKernel::CGaussianKernel(int32_t size, float64_t w)
: CDotKernel(size)
{
	init();
	set_width(w);
}

CGaussianKernel::CGaussianKernel(
	CDotFeatures* l, CDotFeatures* r, float64_t w, int32_t size)
: CDotKernel(size)
{
	init();
	set_width(w);

	init(l,r);
}

CGaussianKernel::~CGaussianKernel()
{
	cleanup();
}

void CGaussianKernel::cleanup()
{
	if (sq_lhs != sq_rhs)
		delete[] sq_rhs;
	sq_rhs = NULL;

	delete[] sq_lhs;
	sq_lhs = NULL;

	CKernel::cleanup();
}

void CGaussianKernel::precompute_squared_helper(float64_t* &buf, CDotFeatures* df)
{
	ASSERT(df);
	int32_t num_vec=df->get_num_vectors();
	buf=new float64_t[num_vec];

	for (int32_t i=0; i<num_vec; i++)
		buf[i]=df->dot(i,df, i);
}

bool CGaussianKernel::init(CFeatures* l, CFeatures* r)
{
	///free sq_{r,l}hs first
	cleanup();

	CDotKernel::init(l, r);
	precompute_squared();
	return init_normalizer();
}

void set_compact_enabled(bool comp)
{
	compact=comp;
}

void get_compact_enabled()
{
	return compact;
}

float64_t CGaussianKernel::compute(int32_t idx_a, int32_t idx_b)
{
	if(!compact) {
		float64_t result=sq_lhs[idx_a]+sq_rhs[idx_b]-2*CDotKernel::compute(idx_a,idx_b);
		return exp(-result/width);
	} else {
		int32_t alen, blen, power;
		alen=((CSimpleFeatures<float64_t>*) lhs)->get_num_features();
		blen=((CSimpleFeatures<float64_t>*) rhs)->get_num_features();
		ASSERT(alen==blen);
		power=alen%2==0?(alen+1):alen;

		float64_t result=sq_lhs[idx_a]+sq_rhs[idx_b]-2*CDotKernel::compute(idx_a,idx_b);
		float64_t result_multiplier=1-(sqrt(result/width))/3;
		if(result_multiplier<=0)
			result_multiplier=0;
		else
			result_multiplier=pow(result_multiplier, power);
		return result_multiplier*exp(-result/width);
	}
}

void CGaussianKernel::load_serializable_post(void) throw (ShogunException)
{
	CKernel::load_serializable_post();
	precompute_squared();
}

void CGaussianKernel::precompute_squared()
{
	if (!lhs || !rhs)
		return;

	precompute_squared_helper(sq_lhs, (CDotFeatures*) lhs);

	if (lhs==rhs)
		sq_rhs=sq_lhs;
	else
		precompute_squared_helper(sq_rhs, (CDotFeatures*) rhs);
}

void CGaussianKernel::init()
{
	set_width(1.0);
	sq_lhs=NULL;
	sq_rhs=NULL;
	m_parameters->add(&width, "width", "Kernel width.");
}
