// Comb filter class declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

#ifndef _comb_
#define _comb_

#include "denormals.h"

class comb
{
public:
	                comb();
	        void    setbuffer(float *buf, int size);
	        void    deletebuffer();
	inline  float   process(float inp);
	        void    mute();
	        void    setdamp(float val);
	        float   getdamp();
	        void    setfeedback(float val);
	        float   getfeedback();
private:
	float   feedback;
	float   filterstore;
	float   damp1;
	float   damp2;
	float   *buffer;
	float   *bufferptr;
	int     bufsize;
	int     bufidx;
};


// Big to inline - but crucial for speed

inline float comb::process(float input)
{
	float output;

	output = undenormalise(*bufferptr);

	filterstore = undenormalise((output*damp2) + (filterstore*damp1));

	*bufferptr = input + (filterstore*feedback);

    bufferptr++;
	if (++bufidx>=bufsize) {
        bufidx = 0;
        bufferptr = buffer;
    }

	return output;
}

#endif //_comb_
