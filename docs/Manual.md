# On power up

When you turn on the TX it will check its flash memory to determine which model (of the 10 available)
you last used and select it and then enter "Use" mode. If your quad is already powered up and
configured for auto-bind you should see the display update with signal strength values and also
the battery voltage being reported by your quad.

The menu display in "Use" mode is very simple consisting primarily of a large countdown timer, 
signal strength numbers and battery voltages. 

*TODO: Show pic of menu in bound state*

The name of the selected model is displayed on the bottom status bar. While the top status bar has 
a speaker icon on the left to indicate if sounds (beeps) are enabled or not. The TX battery voltage 
is displayed on the right of the top status bar.

When entering "Use" mode (whether from menu or on powerup), if the switches `SwA` or `SwD` are "Up", 
or the throttle stick is not fully down, the TX will not arm and an "Alert" dialog is displayed instead.

*TODO: Show pic of menu with "Alert" dialog active*

Flip the switches up and/or push the throttle stick all the way down to exit the "Alert" dialog
and enter "Use" mode. Alternatively you can long press the "Cancel" button to exit the "Alert" dialog
and enter the main menu.

*TODO: Show a picture of the main menu

# The Main Menu
The main menu is where you can edit models and select one to use. You can also use this menu to
configure and review the sticks and aux channels.

You use the push buttons on the FlySky to navigate the menu:

* Ok        - Used to confirm or activate a selection
* Cancel    - Used to cancel an operation or back up out of a menu level
* Up        - Used to move up in a menu list
* Down      - Used to move down in a menu list
* Yaw Trim  - This button can be pushed to the left to back out of a menu option, or to the right to accept a menu option. It can also be used to decrement or increment a value in the menu (such as a letter field or a number field). And it can also be used to move left/right within a multi character (or digit) field.

