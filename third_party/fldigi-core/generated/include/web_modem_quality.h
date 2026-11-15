int modem::get_quality(int mode)
{
	/*  Quality value meanings:
	 * 100% - zero errors, perfect FEC metrics
	 *  90% - zero errors, near-perfect FEC metrics
	 *  75% - zero errors, medium FEC metrics
	 *  50% - very-rare errors, weak FEC metrics
	 *  25% - some errors
	 *  10% - half errors
	 *   0% - mostly errors
	 */

	// Average the quality[] array.
	int average = 0;
	for (int i=0; i<QUALITYDEPTH; i++) {
		average += quality[i];
	}
	average /= QUALITYDEPTH;

	// By default this function returns only quantized values
	// If the second (optional) parameter to this function is anything other than 0, return the actual value.
	if (mode != 0)
		return average;

	// implied else: Scale phase-quality indicatior to quantized values
	if (average > 90) average = 100;
	else if (average > 80) average = 90;
	else if (average > 60) average = 75;
	else if (average > 40) average = 50;
	else if (average > 20) average = 25;
	else if (average > 9) average = 10;
	else average = 0; // Else just round to 0.

	return average;
}


int modem::update_quality(int value, int mode)
{
	static int index=0;

	// Check the passed value, clamp if out of bounds
	if (value > 100)
		value = 100;
	else if (value < 0)
		value = 0;

	// Save the latest passed value to buffer for averaging
	quality[index++] = value;
	if ( QUALITYDEPTH == index) index = 0;

	return get_quality(mode);
}
