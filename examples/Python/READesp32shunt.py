
import sys
import json
from bluepy.btle import Peripheral

# UUIDs for the characteristics
INA_AMPS_UUID = "d2e8ede8-9b31-4478-9fd6-75845cb68b1d"
INA_VOLTS_UUID = "ff100f8c-1266-4309-b472-76bc25e4a62f"
INA_SOC_UUID = "459e9ea4-a335-4a3f-b838-46788fd6bbe4"

def read_ina_data(mac_address):
    try:
        # Connect to the ESP32 device
        device = Peripheral(mac_address)

        # Read AMPs
        amps_char = device.getCharacteristics(uuid=INA_AMPS_UUID)[0]
        amps_data = amps_char.read().decode('utf-8')
        amps = float(amps_data)

        # Read Volts
        volts_char = device.getCharacteristics(uuid=INA_VOLTS_UUID)[0]
        volts_data = volts_char.read().decode('utf-8')
        volts = float(volts_data)

        # Read SOC
        soc_char = device.getCharacteristics(uuid=INA_SOC_UUID)[0]
        soc_data = soc_char.read().decode('utf-8')
        soc = float(soc_data)

        device.disconnect()

        # Return data as dictionary
        return {
            "amps": round(amps, 2),
            "volts": round(volts, 2),
            "soc": round(soc, 2)
        }
    except Exception as e:
        print("Error:", e)
        return None

def main():
    if len(sys.argv) != 2:
        print("Usage: python script.py <ESP32_MAC_ADDRESS>")
        return

    esp32_mac = sys.argv[1]
    ina_data = read_ina_data(esp32_mac)
    if ina_data:
        print(json.dumps(ina_data, indent=4))
    else:
        print("Failed to read INA data.")

if __name__ == "__main__":
    main()
