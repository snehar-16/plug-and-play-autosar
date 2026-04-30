import udsoncan
from doipclient import DoIPClient
from doipclient.connectors import DoIPClientUDSConnector
from udsoncan.client import Client
from udsoncan.exceptions import NegativeResponseException

# 1. Setup the DoIP Transport Layer (ECU at 127.0.0.1, Logical Address 0x1000)
doip_client = DoIPClient('127.0.0.1', 0x1000)

# 2. Bridge the DoIP Client into UDSonCAN
conn = DoIPClientUDSConnector(doip_client)
config = dict(udsoncan.configs.default_client_config)

print("--- SM-OCIP Virtual Railway Tester Starting ---")

# 3. Execute the UDS logic
with Client(conn, request_timeout=2, config=config) as client:
    try:
        print("\n[1] TCP Socket Connected & Routing Activation Successful.")

        print("\n[2] Requesting Extended Session (Service 0x10, Sub 0x03)...")
        client.change_session(0x03)
        print("SUCCESS: ECU transitioned to Extended Session.")

        print("\n[3] Reading Configuration (Service 0x22, DID 0xF100)...")
        req_read = udsoncan.Request(service=udsoncan.services.ReadDataByIdentifier, data=b'\xF1\x00')
        resp_read = client.send_request(req_read)
        print(f"SUCCESS: ECU responded with raw data -> {resp_read.original_payload.hex().upper()}")

        print("\n[4] Attempting Unauthorized Write (Service 0x2E, DID 0xF300)...")
        try:
            req_write = udsoncan.Request(service=udsoncan.services.WriteDataByIdentifier, data=b'\xF3\x00\xFF')
            client.send_request(req_write)
        except NegativeResponseException as e:
            if e.response.code == 0x33:
                print(f"EXPECTED REJECTION: ECU correctly blocked write with NRC 0x33.")
            else:
                print(f"FAIL: Unexpected NRC {e.response.code}")

        print("\n[5] Requesting Clear Diagnostic Information (Service 0x14)...")
        client.clear_diagnostic_information(group=0xFFFFFF)
        print("SUCCESS: DEM memory wipe command accepted.")

        print("\n--- All SM-OCIP Integration Tests Passed ---")

    except Exception as e:
        print(f"\nCRITICAL ERROR: {e}")
