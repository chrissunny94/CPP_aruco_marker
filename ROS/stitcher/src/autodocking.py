#!/usr/bin/python
#import argparse
#import imutils
import sys
import rospy
import cv2 , math

from std_msgs.msg import String
from sensor_msgs.msg import Image
import numpy as np
from cv_bridge import CvBridge, CvBridgeError
from std_msgs.msg import String
from geometry_msgs.msg import Twist
from sensor_msgs.msg import Range



class Detect_pattern:
    def __init__(self , debug = False):
        self.image_pub = rospy.Publisher("image_topic_2", Image, queue_size=10)
        self.twist_pub = rospy.Publisher('cmd_vel', Twist, queue_size=10)

        self.debug = debug


        self.bridge = CvBridge()
        self.image_sub = rospy.Subscriber("/image_raw", Image, self.callback)
        self.twist = Twist()

    def callback(self, data):
        try:
            cv_image = self.bridge.imgmsg_to_cv2(data, "bgr8")
        except CvBridgeError as e:
            print(e)

        (rows, cols, channels) = cv_image.shape
        if cols > 60 and rows > 60:
            cv2.circle(cv_image, (50, 50), 10, 255)

        if self.debug:
            cv2.imshow("Image window", cv_image)
            cv2.waitKey(1)

        try:
            x , area = self.colour_detect(cv_image)

            if x:
                x , area = self.PoseEstimator(cv_image)
                print "X=",  x,",Area =", area
                if x:
                    self.drive_motor(x , area)
            else:
                self.drive_motor(0,0)
            pass
        except:
            print "Failed to detect"

    def PoseEstimator(self, frame):
        # convert the frame to grayscale, blur it, and detect edges
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        blurred = cv2.GaussianBlur(gray, (7, 7), 0)
        edged = cv2.Canny(blurred, 50, 150)
        # find contours in the edge map
        # print cv2.findContours(edged.copy(), cv2.RETR_EXTERNAL,cv2.CHAIN_APPROX_SIMPLE)
        (_, cnts, _) = cv2.findContours(edged.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        Area = 0
        # loop over the contours
        for c in cnts:
            # approximate the contour
            peri = cv2.arcLength(c, True)
            approx = cv2.approxPolyDP(c, 0.01 * peri, True)

            # ensure that the approximated contour is "roughly" rectangular
            if len(approx) >= 4 and len(approx) <= 6:
                # compute the bounding box of the approximated contour and
                # use the bounding box to compute the aspect ratio
                (x1, y1, w, h) = cv2.boundingRect(approx)
                # print x1
                # print y1

                aspectRatio = w / float(h)

                # compute the solidity of the original contour
                Area = []
                area = cv2.contourArea(c)
                Area = max(area, Area)

                hullArea = cv2.contourArea(cv2.convexHull(c))
                solidity = area / float(hullArea)

                # compute whether or not the width and height, solidity, and
                # aspect ratio of the contour falls within appropriate bounds
                keepDims = w > 25 and h > 25
                keepSolidity = solidity > 0.9
                keepAspectRatio = aspectRatio >= 0.8 and aspectRatio <= 1.2

                # ensure that the contour passes all our tests
                if keepDims and keepSolidity and keepAspectRatio:
                    # draw an outline around the target and update the status
                    # text
                    if self.debug:
                        cv2.drawContours(frame, [approx], -1, (0, 0, 255), 4)
                    status = "Target(s) Acquired"
                    # This will give you the Pixel location of the rectangular box
                    rc = cv2.minAreaRect(approx[:])
                    # print rc,'rc'
                    box = cv2.boxPoints(rc)
                    pt = []
                    for p in box:
                        val = (p[0], p[1])
                        pt.append(val)
                        # print pt,'pt'
                        cv2.circle(frame, val, 5, (200, 0, 0), 2)

                    M = cv2.moments(approx)
                    (cX, cY) = (int(M["m10"] / M["m00"]), int(M["m01"] / M["m00"]))
                    (startX, endX) = (int(cX - (w * 0.15)), int(cX + (w * 0.15)))
                    (startY, endY) = (int(cY - (h * 0.15)), int(cY + (h * 0.15)))
                    
                    if self.debug:
                        cv2.line(frame, (startX, cY), (endX, cY), (0, 0, 255), 3)
                        cv2.line(frame, (cX, startY), (cX, endY), (0, 0, 255), 3)
                    # print "cX", cX
                    if self.debug:
                        cv2.imshow("Image window", frame)
                        cv2.waitKey(3)

                    # 2D image points. If you change the image, you need to change vector
                    image_points = np.array([pt], dtype="double")
                    # print image_points
                    size = frame.shape
                    # 3D model points.
                    model_points = np.array([
                        (0.0, 0.0, 0.0),  # Rectangle center
                        (0.0, 13.6, 0.0),  #
                        (13.6, 13.6, 0.0),  #
                        (13.6, 0.0, 0.0),  #

                    ])

                    # Camera intrinsic parameters

                    focal_length = size[1]
                    center = (size[1] / 2, size[0] / 2)
                    camera_matrix = np.array(
                        [[focal_length, 0, center[0]],
                         [0, focal_length, center[1]],
                         [0, 0, 1]], dtype="double"
                    )

                    # print "Camera Matrix :\n {0}".format(camera_matrix)
                    criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)

                    dist_coeffs = np.zeros((4, 1))  # Assuming no lens distortion
                    (success, rotation_vector, translation_vector) = cv2.solvePnP(model_points, image_points,
                                                                                  camera_matrix,
                                                                                  dist_coeffs, criteria)

                    rotationMatrix = np.zeros((3, 3), np.float32)

                    # print "Rotation Vector:\n {0}".format(rotation_vector)
                    # print "Translation Vector:\n {0}".format(translation_vector)


                    # Rodrigues to convert it into a Rotation vector
                    print rotation_vector
                    dst, jacobian = cv2.Rodrigues(rotation_vector)
                    # print "Rotation matrix:\n {0}".format(dst)
                    # sy = math.sqrt(dst[0, 0] * dst[0, 0] + dst[1, 0] * [1, 0])
                    A = dst[2, 1] * dst[2, 2]
                    B = dst[1, 0] * dst[0, 0]
                    return cX , translation_vector[2]
        return 0 , 0

    

    def colour_detect(self , img ):

        img = cv2.resize(img, (340, 220))
        lowerBound = np.array([33, 80, 40])
        upperBound = np.array([102, 255, 255])

        kernelOpen = np.ones((5, 5))
        kernelClose = np.ones((20, 20))
        # convert BGR to HSV

        imgHSV = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)


        # create the Mask
        mask = cv2.inRange(imgHSV, lowerBound, upperBound)
        # morphology

        maskOpen = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernelOpen)
        maskClose = cv2.morphologyEx(maskOpen, cv2.MORPH_CLOSE, kernelClose)

        maskFinal = maskClose
        _, conts, h = cv2.findContours(maskFinal.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
        if self.debug:
            cv2.drawContours(maskClose, conts, -1, (255, 0, 0), 3)
        x = None
        for i in range(len(conts)):
            x, y, w, h = cv2.boundingRect(conts[i])
            cv2.rectangle(img, (x, y), (x + w, y + h), (0, 0, 255), 2)
            # cv2.cv.PutText(cv2.cv.fromarray(img), str(i+1),(x,y+h),font,(0,255,255))
        if self.debug:
            cv2.imshow("maskClose", maskClose)
            cv2.imshow("maskOpen", maskOpen)
            cv2.imshow("mask", mask)
            cv2.imshow("cam", img)
            cv2.waitKey(10)


        if x:
            return x , w*h
        else:
            return 0 , 0
    def drive_motor(self, x , area):
        if x:
            self.twist.linear.x = self.translate(area, -50, 150, -.2, .5)
        else:
            self.twist.linear.x = 0

        self.twist.linear.y = 0
        self.twist.linear.z = 0
        self.twist.angular.x = 0
        self.twist.angular.y = 0

        if area:
            self.twist.angular.z = self.translate(x, 100, 500, .05, -.05)
        else:
            self.twist.angular.z = 0 
        # print "Twist values are " , self.twist
        try:
            self.twist_pub.publish(self.twist)
            print self.twist
        except:
            print "failed to publish"

    def translate(self, value, leftMin, leftMax, rightMin, rightMax):
        # Figure out how 'wide' each range is
        leftSpan = leftMax - leftMin
        rightSpan = rightMax - rightMin

        # Convert the left range into a 0-1 range (float)
        valueScaled = float(value - leftMin) / float(leftSpan)

        # Convert the 0-1 range into a value in the right range.
        return rightMin + (valueScaled * rightSpan)




###############################################################################################################################

def main(args):
  ic = Detect_pattern(debug=False)
  rospy.init_node('listner', anonymous=True)
  rate = rospy.Rate(rospy.get_param('~hz', 10))
  try:
    rospy.spin()
    rate.sleep()
  except KeyboardInterrupt:
    print("Shutting down")

  cv2.destroyAllWindows()

if __name__ == '__main__':
    print cv2.__version__
    main(sys.argv)

