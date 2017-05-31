# LARPGeigercounter
Fake Geigercounter using Arduino, HC-SR04 and a speaker.

Basically, the Arduino will output clicks ("tocks") on the attached speaker in order to simulate a geiger counter.
The intended purpose is use as a prop on endtime LARP cons or such.

The tock rate will increase when you hold the sensor closer to an object.
You can connect a switch to the Arduino which will increase the tock-rate when pressed. I call this the "OMGTheresRadiationHereWeAreGonnaDie-Button". ;)

The sketch provides plenty of possibilities for tuning the behaviour (click rates, rate of increase when closing in on an objects, ...)

Speaker should be driven with a small NPN (BC337) or similar.

# TODO
  * Make the tock rate increase when closing in on objects exponential.
