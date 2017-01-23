#TWAMP-Measurement-IoT
Active performance measurments for IoT devices

The following repository contains all the code and logs from our project Active performance measurments for IoT devices that was done as part of the course Computer Networks III at Uppsala University in the spring of 2016.

The goal of the project was to implement the TWAMP protocol on sensor nodes using Contiki as the operating system. The nodes used were Zolertia Z1 motes.

## Setup
In order to run the following code it is highly recommended that you use InstantContiki inside a VirtualBox. Other options may work as well but we cannot guarantee this.

To be able to compile the project properly without having to change the Makefile the directory structure needs to be the following:
    ~/repos/TWAMP-Measurment-IoT/
    
(!)When starting your VirtualBox it should be started as an administrator. This is important otherwise you will get an error when trying to flash the nodes.

## Flashing the nodes
The flash the nodes with the code we have written the following steps have to be taken.

1. Navigate to the main folder of the git repo (perferably ~/repos/TWAMP-Measurment-IoT/).
2. Create a save target for the Z1 mote by running `make TARGET=z1 savetarget`, this isn't strictly nescessary however if you are only using Z1 motes then this saves you some time since you won't have to run `TARGET=z1` with each command.
3. Connect one sensor node to the computer using a USB cable.
4. Make sure the node is properly connected by writing `make z1-motelist`. This will display a list of all connected motes and their address in the form of `/dev/ttyUSB0`
5. In order to flash the mote use the following command `make MOTES=/dev/ttyUSB0 twamp_light_sender.upload && make MOTES=/dev/ttyUSB0 reset`. This will flash the file `twamp_light_sender.c`into the memory of the mote and reset it.
6. Connect the other mote and run `make MOTES=/dev/ttyUSB1 twamp_light_reflector.upload && make MOTES=/dev/ttyUSB1 reset`.
7. In order to see the output from a mote you can run `make MOTES=/dev/ttyUSBX login` where X is the mote you want to login to. 

## Troubleshooting
If you are expriencing problems with login to a mote then you need to replace the `serialdump-linux` file in the folder `~/contiki/tools/sky/serialdump-linux` with a new one found here:  https://github.com/contiki-os/contiki/blob/master/tools/sky/serialdump-linux and in addition to this you also need to run the following command `sudo adduser $USER dialout`.

If you are having trouble when flashing the motes due to a syncronization error. Keep trying! This is an error that we experienced during our development process that we believe has something to due with the speed of the USB port. Over several computers we had one that always failed when flashing, one that always succeeded and some that had between 20-70% success rate. So your milage may wary!

If you are having problems with the motes not gaining proper local ipv6 addresses you need to look into burning a new node id into your mote since it might not work properly. Instead of describing the process here we point you towards the excellent book IoT in 5 days found at http://wireless.ictp.it/school_2015/book/book.pdf and in that book you should look into chapter 7.1 which should cover everything you need to know.
