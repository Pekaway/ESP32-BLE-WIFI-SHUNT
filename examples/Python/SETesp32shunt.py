
import sys
from bluepy.btle import Peripheral

# UUIDs for the characteristics
WRITE_SOC_UUID = "63d58a25-c22b-4586-b297-f1e310b7b0bc"
WRITE_AMPHOURS_UUID = "38f2bd70-d659-4970-86c1-061e24700a6e"

def write_data(mac_address, char_uuid, data):
    try:
        device = Peripheral(mac_address)
        characteristic = device.getCharacteristics(uuid=char_uuid)[0]
        characteristic.write(data.encode())
        device.disconnect()
        return True
    except Exception as e:
        print("Error:", e)
        return False

def main():
    if len(sys.argv) != 4:
        print("Usage: python set_soc_amphours.py <ESP32_MAC_ADDRESS> <BATTERY_AMPHOURS> <SOC>")
        return

    esp32_mac = sys.argv[1]
    battery_amphours = sys.argv[2]
    soc = sys.argv[3]

    # Write Battery AMPHOURS
    if write_data(esp32_mac, WRITE_AMPHOURS_UUID, battery_amphours):
        print("Battery AMPHOURS successfully written.")
    else:
        print("Failed to write Battery AMPHOURS.")

    # Write SOC
    if write_data(esp32_mac, WRITE_SOC_UUID, soc):
        print("SOC successfully written.")
    else:
        print("Failed to write SOC.")

if __name__ == "__main__":
    main()





