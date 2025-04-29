from pysabertooth import Sabertooth

saber = Sabertooth('/dev/ttyACM0', baudrate=9600, address=128, timeout=0.1)

# drive(number, speed)
# number: 1-2
# speed: -100 - 100
saber.drive(1, 50)
saber.drive(2, -75)
saber.stop()
