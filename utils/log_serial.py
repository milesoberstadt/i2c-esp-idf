import serial
import time

# Configuration
port = '/dev/tty.usbserial-A50285BI'  
baudrate = 4800  # Set the baudrate according to your device specification

def log_serial_data():
    try:
        # Open the serial port
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Connected to {port} at {baudrate} baud.")

        while True:
            # Read data from the serial port
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                # Log data to console with timestamp
                timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
                print(f'{timestamp} - {data.hex()}')

    except Exception as e:
        print(f"Error: {e}")
    finally:
        if ser.is_open:
            ser.close()
        print("Serial port closed.")

if __name__ == "__main__":
    log_serial_data()
