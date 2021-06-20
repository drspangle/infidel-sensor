# Inline Filament Diameter Estimator, Lowcost (InFiDEL)

<p xmlns:dct="http://purl.org/dc/terms/" xmlns:vcard="http://www.w3.org/2001/vcard-rdf/3.0#">
  <a rel="license"
     href="http://creativecommons.org/publicdomain/zero/1.0/">
    <img src="https://licensebuttons.net/p/zero/1.0/80x15.png" style="border-style: none;" alt="CC0" />
  </a>
  Originally created by Thomas Sanladerer
</p>

*A cheap, yet precise filament diameter sensor, intended to compensate for filament diameter deviations in real-time.*

The InFiDEL is a cheap (< $5) filament diameter sensor intended for use with FDM 3d printers.
The sensor can be calibrated to provide surprisingly precise filament diameter readings in real-time.
The main idea is to use the sensor to correct for filament diameter deviations while printing.

Based on this proof-of-concept: https://www.youmagine.com/designs/filament-diameter-sensor

## Documentation

Documentation for this project is now available to read - [https://drspangle.github.io/infidel-sensor/](https://drspangle.github.io/infidel-sensor/)

If you need to build the documentation yourself locally run the following command after cloning the repository in the root directory

`sphinx-build -b html docs-src/source docs`
