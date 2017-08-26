import RPi.GPIO as GPIO
import getopt
import sys

def main(argv):
    result = {'pin': '', 'level': ''}
    opts, args = getopt.getopt(argv, "", ["pin=", "level="])
    for opt, arg in opts:
        if opt == "--pin":
            result['pin'] = arg
        elif opt == "--level":
            result['level'] = arg
    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BOARD)
    pin = int(result['pin'])
    GPIO.setup(pin, GPIO.OUT)

    if result['level'] == 'high':
        GPIO.output(pin, GPIO.HIGH)
    elif result['level'] == 'low':
        GPIO.output(pin, GPIO.LOW)

if __name__ == '__main__':
    main(sys.argv[1:])
