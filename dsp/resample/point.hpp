#ifdef NALL_DSP_INTERNAL_HPP

void DSP::resamplePoint() {
  while(resampler.fraction <= 1.0) {
    double channel[settings.channels];

    for(unsigned n = 0; n < settings.channels; n++) {
      double a = buffer.read(n, -1);
      double b = buffer.read(n, -0);

      double mu = resampler.fraction;

      channel[n] = mu < 0.5 ? a : b;
    }

    resamplerWrite(channel);
    resampler.fraction += resampler.step;
  }

  buffer.rdoffset++;
  resampler.fraction -= 1.0;
}

#endif
