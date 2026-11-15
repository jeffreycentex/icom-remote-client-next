void modem::ModulateXmtr(double* buffer,int len) {
if(!buffer||len<1)return;
tx_sample_rate=samplerate;tx_sample_count+=len;
const double SIGLIMIT=0.95;
	int num = ( (samplerate * progdefaults.SoftStart / 1000) / symlen + 1) * symlen;

	if (sig_start && sig_smpl < num) {
		for (int i = 0; i < len; i++) {
			double coeff = (0.5 * (1.0 - cos (M_PI * sig_smpl / num)));;
			buffer[i] *= coeff;
			++sig_smpl;
			if (sig_smpl >= num) {
				sig_smpl = 0;
				sig_start = false;
				break;
			}
		}
	}

	if (sig_stop && sig_smpl < num) {
		for (int i = 0; i < len; i++) {
			double coeff = 1.0 - (0.5 * (1.0 - cos (M_PI * sig_smpl / num)));;
			buffer[i] *= coeff;
			++sig_smpl;
			if (sig_smpl >= num) {
				sig_smpl = 0;
				sig_stop = false;
				break;
			}
		}
	}

	double mult = pow(10, progStatus.txlevel / 20.0);
	if (mult > SIGLIMIT) mult = SIGLIMIT;
	for (int i = 0; i < len; i++) {
		buffer[i] *= mult;
		if (buffer[i] < -SIGLIMIT) buffer[i] = -SIGLIMIT;
		if (buffer[i] >  SIGLIMIT) buffer[i] = SIGLIMIT;
	}

web_tx_audio(buffer,len);
}
