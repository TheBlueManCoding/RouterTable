# RouterTable

This Project is a command unit for a router table:
 - control the router bit height(Z axis)
 - control the fence position (Y axis)

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
