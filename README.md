# CloneSD-electronic
Electronic project for labelec 42 - copy an sd card to another one using pic/spi

pic32mx340f512h
2 sd card
lcd

# The lowlevel project:
That's the real clone. This project allow to clone all an sd1 to sd2 that means even the name of sd card will be the same. in this case we need same card storage capacity and if there is any document in the sd2 it will disappear.

# The sd clone project:
This project will copy sd1 to sd2 without erasing any document. it will keep all the document and add what is find in sd1.
In case of full memory card will ask for format because of this we are using an lcd to show alert message.

From the second project you can create so many other projects like reading what's in an sd card, or make a music player,.. you can create any project need an access to sd card.

# GOOD LUCK ! 
