# RouterTable
CAUTION: This project only builds in VisualMicro plugin for Atmel Studio at the moment.
         For prebuild versions take a look into the bin folder. if you miss a configuration i can add it.

This Project is a command unit for a router table:
 - control the router bit height(Z axis)
 - control the fence position (Y axis)

You can find the files for the housing and some pictures here: https://www.thingiverse.com/thing:4841289

You can choose which axes you want to use in the Config.h file. The software is tested with Y+Z and the Y only configuration.

A grbrl board(V1.1) has to be used for for the motor axis control and configuration.
This way its easy to configurate the project for every hardware which is supported by grbl.
Required configuration:
 - enable homing and limit switches
 - compile the code with only the used axis enabled --> the stock version will give you an homing error because we dont use all three axis.

You do not enter the fence position, you just enter what to do(for example a dado on a specific position) and the software calculates the cutting passes for you. 
Its also possible to calculate and cut finger joints.

ALWAYS BLOCK THE ROUTER FENCE AFTER MOVING IT, FOR YOUR OWN SAFETY! THE SOFTWARE MAY CONTAIN BUGS, 
THIS CAN BE DANGEROUS IF YOUR HANDLS ARE NEAR THE CUTTING ROUTER BIT IF THE FENCE BEGINS TO MOVE!!

Because there is no complete documentation at the moment, feel free to ask me if you are interested in the project.

Used libraries:

- https://github.com/Jomelo/LCDMenuLib             Version 2.1.4
- https://github.com/johnrickman/LiquidCrystal_I2C Version 1.1.1
- https://playground.arduino.cc/Code/Keypad/       Version 3.1.1

## Finished router table Version 1
This version is only a moving fence and follows the example of the Ready2Rout router fence: https://www.rockler.com/ready2rout-the-first-ever-electronic-router-fence
It was my inspiration when i started this project - because i couldn't buy it in germany.
The price and mechanical effort for this version is low - but it needs much space behind the router table.

Images will come up soon after the rebuild of the electronics.

## Finished router table Version 2
![RouterTable V2](https://github.com/TheBlueManCoding/RouterTable/blob/master/doc/Images/RouterTableV2-Complete.JPG)

