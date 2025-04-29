

import arduinoserial
import serial.tools.list_ports as port


import rospy, time, math, cv2, sys
from std_msgs.msg import String, Float32, Int32
from geometry_msgs.msg import Twist, Pose, PoseStamped, PoseWithCovariance, PoseWithCovarianceStamped, Transform
from tf import transformations

varS=None





class Connect_Bot:
    def __init__(self, baud, usb_dev ):
		portlist = list(port.comports())
		address = ''
		for p in portlist:
  			print p
			if 'CP2102' in str(p):
				address = str(p).split(" ")
		print address[0]

		self.arduino = arduinoserial.SerialPort(address[0], baud)
		self.arduino.write('F:500')
		self.arduino.write('F:-500')
		self.key_status = []
		self.pre_key_status = []
		self.key_list = ['Key.up', 'Key.down', 'Key.left', 'Key.right', 'Key.space']
		for i in range(0, len(self.key_list)):
			self.key_status.append(0)
			self.pre_key_status.append(9)
		self.speed_status_up = 0
		self.speed_status_down = 0
        # Collect events until released



    def key_response(self,key_status):
        # print key_status
        # stop
        if key_status == [0, 0, 0, 0, 0]:
            return 'S:00\n'
        # move forward
        elif key_status == [1, 0, 0, 0, 0]:
            return 'D:2047\n'
        # move backward
        elif key_status == [0, 1, 0, 0, 0]:
            return 'D:-2047\n'


        # move left forward
        elif key_status == [1, 0, 1, 0, 0]:
            return 'T:-1000\n'
        # move left backward
        elif key_status == [0, 1, 1, 0, 0]:
            return 'T:1000\n'
        # left turn 90deg
        elif key_status == [0, 0, 1, 0, 0]:
            return 'T:-2000\n'



        # move right forward
        elif key_status == [1, 0, 0, 1, 0]:
            return 'T:1000\n'
        # move right backward
        elif key_status == [0, 1, 0, 1, 0]:
            return 'T:-1000\n'
        # right turn 90deg
        elif key_status == [0, 0, 0, 1, 0]:
            return 'T:2000\n'



        elif key_status == [0, 0, 0, 0, 1]:
            return 'S:0\n'

        else:
            return 'S:0\n'


    def KeyCheck(stdscr):
    	stdscr.keypad(True)
    	stdscr.nodelay(True)

    	k = None
    	global std
    	std = stdscr

    	#publishing topics
    	pubVel   = rospy.Publisher('/platform_control/cmd_vel', Twist)

    	# While 'Esc' is not pressed
    	while k != chr(27):
        	# Check no key
        	try:
            		k = stdscr.getkey()
        	except:
            		k = None

  
      		
        	if k == " ":
            		robotTwist.linear.x  = 0.0           
            		robotTwist.angular.z = 0.0
        	if k == "KEY_LEFT":
            		robotTwist.angular.z += radStep
        	if k == "KEY_RIGHT":
            		robotTwist.angular.z -= radStep
        	if k == "KEY_UP":
            		robotTwist.linear.x +=  linStep
        	if k == "KEY_DOWN":
            		robotTwist.linear.x -= linStep

        	robotTwist.angular.z = min(robotTwist.angular.z,deg2rad(90))
        	robotTwist.angular.z = max(robotTwist.angular.z,deg2rad(-90))
        	robotTwist.linear.x = min(robotTwist.linear.x,1.0)
        	robotTwist.linear.x = max(robotTwist.linear.x,-1.0)
        	pubVel.publish(robotTwist)

        	showStats()
        	time.sleep(0.1)

    	stdscr.keypad(False)
    	rospy.signal_shutdown("Shutdown Competitor")



    


if __name__ == "__main__":
		connect_bot = Connect_Bot(baud, usb_dev)
		name = 'tapbot'
		subTopic =  '/joy_teleop/cmd_vel'
		print "Started topic" , name
		rospy.init_node(name)

		print( "Subscribed to topic", subTopic)
		sub=rospy.Subscriber(subTopic, Twist, fnc_callback)
		pub=rospy.Publisher('cmd_vel', Twist, queue_size=1)
		rate=rospy.Rate(10)








