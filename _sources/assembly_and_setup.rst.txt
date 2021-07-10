Assembly and setup
==================

|Image|_

Calibration
-----------
- Flash board with calibration.ino
- Connect a host via I2C (see provided host-example.ino)
- Insert each calibration drill bit shafts and replace the lookup table entries in driver.ino with your own measurements

  * Measure the actual diameter of the shafts and fill second column
  * Use the host's output multiplied by 1000 to fill first column

- Finally, flash driver.ino with your values filled out
- Hub should now output exact diameter values

    Note: Analog output is currently not enabled in driver.ino (I2C and FAULT pin only)

.. |Image| image:: https://img.youtube.com/vi/RYgdLPe_T0c/0.jpg
           :alt: Assembly and Setup Instructions on Youtube
.. _Image: https://www.youtube.com/watch?v=RYgdLPe_T0c
