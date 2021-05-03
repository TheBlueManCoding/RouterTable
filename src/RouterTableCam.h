/*
 * RouterFenceCam.h
 *
 *  Created on: 16.12.2016
 *      Author: Martin Danisch
 */

#ifndef ROUTERFENCECAM_H_
#define ROUTERFENCECAM_H_

#include "Config.h"

#ifndef __GNUC__
#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#else
#include <math.h>
#endif

#define AS_INT(value) ((value)*10.0)
#define AS_DOUBLE(value) ((value)/10.0)

class RouterFenceCam {
	public:

	struct Dado {
		double position;
		double width;
		double depth;
		bool reversedOrder;  // the dado needs to be cut in reversed order
	};
	
	struct Pass {
		int16_t posY;
		int16_t posZ;
	};

	/**
	 * calc the passes required for one dado of specified width and depth
	 * 
	 * @param routerBitWidth the width of the router bit
	 * @param routerBitMaxDepth the maximum cutting depth for one pass
	 * @param dado the dado
	 * @param passes the calculated passes
	 * @param maxPasses max. size of the pass buffer
	 * @param passCount the calculated pass count to get the dado
	 * 
	 * @return bool true if dado possible
	 * @the reference is the back of the router bit(-> pass->posY = 10mm makes a groove of the router bit widt at position 10mm to e.g. 20mm)
	 */
	static bool calculateDadoPasses(double routerBitWidth, double routerBitMaxDepth, Dado dado, Pass passes[], int maxPasses, int& passCount) {

		passCount=0;

		// calculate first pass
		if (dado.position == 0.0) {
			// cut the small pass in full depth
			passes[passCount].posY = AS_INT(-routerBitWidth + 0.5);
			passes[passCount].posZ = AS_INT(dado.depth);
			passCount++;
		} else {
			
			// calculate the number of full z passes(the last cut is the lightest pass)
			// this is the best to use the most of your cutters life.
			double depth = 0;
			if (dado.depth > routerBitMaxDepth) {				
				size_t passesZ = dado.depth / routerBitMaxDepth;
				while(passesZ--) {
					depth += routerBitMaxDepth;
					passes[passCount].posY = AS_INT(dado.position);
					passes[passCount].posZ = AS_INT(depth);
					passCount++;
				}				
			}
			
			if(depth < dado.depth) {
				passes[passCount].posY = AS_INT(dado.position);
				passes[passCount].posZ = AS_INT(dado.depth);
				passCount++;
			}
		}

		if (dado.width < routerBitWidth) {
			if (dado.position > 0) {
				return false;
			}
		}

		// calculate other passes width
		double rest = dado.width - (AS_DOUBLE(passes[passCount-1].posY) + routerBitWidth - dado.position);
		double maxPassWidth = routerBitWidth;
		double restPasses = ceil(rest / maxPassWidth); // calculate passes, round up to next full pass
		double passWidth = rest / restPasses;
		
		// calculate passes until the full width is reached
		while(restPasses--) {
			double depth = 0;
			double posY = passes[passCount-1].posY + AS_INT(passWidth);
			
			if (dado.depth > routerBitMaxDepth) {
				// calculate the number of full z passes(the last cut is the lightest pass)
				// this is the best to use the most of your cutters life.
				size_t passesZ = dado.depth / routerBitMaxDepth; 			
				while(passesZ--) {
					depth += routerBitMaxDepth;
					passes[passCount].posY = posY;
					passes[passCount].posZ = AS_INT(depth);
					passCount++;
				}
			}
			
			if(depth < dado.depth) {
				passes[passCount].posY = posY;
				passes[passCount].posZ = AS_INT(dado.depth);
				passCount++;
			}
		}
		return true;
	}

	static bool calculateFingerGrooves(bool isMale, double routerBitWidth, double sheetWidth, double clearanceWidth, 
				double depth, int numberOfFingers, Dado grooves[], int maxGrooves, int& groovesCount) {
		groovesCount  = 0;

		double fingerWidth = sheetWidth / numberOfFingers;

		if (fingerWidth < routerBitWidth) {
			return false;
		}

		groovesCount=0;

		if (!isMale) {
			for (int i=0; i<numberOfFingers; i+=2) {
				grooves[groovesCount].position = i*fingerWidth;
				grooves[groovesCount].width = fingerWidth;
				grooves[groovesCount].depth = depth;
				groovesCount++;
			}
		} else {
			for (int i=1; i<numberOfFingers; i+=2) {
				grooves[groovesCount].position = i*fingerWidth;
				grooves[groovesCount].width = fingerWidth + clearanceWidth;
				grooves[groovesCount].depth = depth;
				groovesCount++;
			}
		}

		return true;
	}

	/**
	 * recalculate dados for reversed order cuts, if necessary.
	 * @param sheetWidth        the real sheet width
	 * @param maxTravel         the maximum travel of the fence
	 * @param routerBitWidth    the router bit width
	 * @param grooves           the original grooves in linear order
	 * @param groovesCount      the original grooves count before call and the recalculated number after call
	 * @param maxGrooves        the max number grooves
	 * @return @return true if calculation was possible and the cuts are valid
	 */
	static int calcDadosReversedOrder(double sheetWidth, double maxTravel, double routerBitWidth, Dado grooves[], int& groovesCount, int maxGrooves) {

		for (int i=0; i<groovesCount; i++) {
			Dado dadosReversed[2];
			int reversedDadoCount = 0;
			if (!calcDadoReversedOrder(sheetWidth, maxTravel, routerBitWidth, grooves[i], dadosReversed, reversedDadoCount)) {
				return false;
			}

			if (reversedDadoCount == 1) {
				// just do a copy
				memcpy(&grooves[i], &dadosReversed[0], sizeof(Dado));
			} else if (reversedDadoCount == 2) {
				// copy 1st
				memcpy(&grooves[i], &dadosReversed[0], sizeof(Dado));
				// check for place
				if (!(groovesCount < maxGrooves)) {
					return false;
				}
				// insert 1 new element
				int numCopy = groovesCount - i - 1;
				memmove(&grooves[i+2], &grooves[i+1], numCopy*sizeof(Dado));
				memcpy(&grooves[i+1], &dadosReversed[1], sizeof(Dado));

				i++; groovesCount++;
			} else {
				return false; // internal error
			}
		}
		return true;
	}

	/**
	 * Check if dado needs reversed cutting and calculate the reversed dado if so.
	 * The dado will be cut in two half's if necessary.
	 * @param sheetWidth        the real sheet width
	 * @param maxTravel         the maximum travel of the fence
	 * @param routerBitWidth    the router bit width
	 * @param originalDado      the original dado
	 * @param dadosReversed     the reversed dados to cut
	 * @param reversedDadoCount the number of reversed dados(1 or 2)
	 * @return true if calculation was possible and the cuts are valid
	 */
	static bool calcDadoReversedOrder(double sheetWidth, double maxTravel, double routerBitWidth, Dado originalDado, Dado dadosReversed[2], int& reversedDadoCount) {

		double firstPosition = originalDado.position;
		double lastPosition = originalDado.position + originalDado.width;

		if (originalDado.width < routerBitWidth && originalDado.position == 0.00) {
			dadosReversed[0].position      = originalDado.position;
			dadosReversed[0].width         = originalDado.width;
			dadosReversed[0].depth         = originalDado.depth;
			dadosReversed[0].reversedOrder = false;
			reversedDadoCount = 1;
			return true; // the first dado is allowed to be smaller than the router bit width
		}

		// allow a little overcut, max is the router bit width.
		if (lastPosition > sheetWidth + routerBitWidth) {
			return false; // cut is not possible
		} else {
			if (lastPosition <= maxTravel) { // normal cut
				dadosReversed[0].position      = originalDado.position;
				dadosReversed[0].width         = originalDado.width;
				dadosReversed[0].depth         = originalDado.depth;
				dadosReversed[0].reversedOrder = false;
				reversedDadoCount = 1;
			} else if (firstPosition >= maxTravel && lastPosition >= maxTravel) { // reversed cut
				dadosReversed[0].position      = sheetWidth - lastPosition;
				dadosReversed[0].width         = originalDado.width;
				dadosReversed[0].depth         = originalDado.depth;
				dadosReversed[0].reversedOrder = true;
				reversedDadoCount = 1;
			} else { // split cut in normal and reversed cut
				dadosReversed[0].position      = originalDado.position;
				dadosReversed[0].width         = maxTravel - originalDado.position; // cut to the middle
				dadosReversed[0].depth         = originalDado.depth;
				dadosReversed[0].reversedOrder = false;

				dadosReversed[1].position      = sheetWidth - lastPosition;
				dadosReversed[1].width         = originalDado.width - dadosReversed[0].width; // cut the rest
				dadosReversed[1].depth         = originalDado.depth;
				dadosReversed[1].reversedOrder = true;

				// cut a little over the middle, because the full dado is bigger than the router bit
				if(dadosReversed[0].width < routerBitWidth) {
					dadosReversed[0].width = routerBitWidth;
				}
				if(dadosReversed[1].width < routerBitWidth) {
					dadosReversed[1].width = routerBitWidth; // cut a little over the middle, because it must be cut away
				}
				reversedDadoCount = 2;
			}

			return true;
		}
	}
};

#endif /* ROUTERFENCECAM_H_ */
