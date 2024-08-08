import serial
import time

# Configuration
port = '/dev/tty.usbserial-A50285BI'  
baudrate = 4800  # Set the baudrate according to your device specification
speed_frame = bytes([0x01 ,0x03 ,0x00 ,0x00 ,0x00 ,0x01 ,0x84 ,0x0A]) # SPEED
direction_frame = bytes([0x02 ,0x03 ,0x00 ,0x00 ,0x00 ,0x02 ,0xC4 ,0x38]) # DIRECTION

def main():
    try:
        # Open the serial port
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Connected to {port} at {baudrate} baud.")

        while True:
            print("Speed :")
            send_inquiry(ser, speed_frame)

            time.sleep(0.2)

            print("Direction :")
            send_inquiry(ser, direction_frame)


    except Exception as e:
        print(f"Error: {e}")
    finally:
        if ser.is_open:
            ser.close()
        print("Serial port closed.")

def send_inquiry(ser, frame):

    # Send the inquiry frame
    ser.write(frame)
    
    # Wait for a moment to receive the response
    time.sleep(0.5)
    
    # Read the response
    if ser.in_waiting > 0:
        response = ser.read(ser.in_waiting)
        # Log data to console with timestamp
        timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
        print(f'{timestamp} - {response.hex()}')


if __name__ == "__main__":
    main()
