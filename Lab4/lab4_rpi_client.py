#!/usr/bin/env python3
"""Lab4 Pi central: set the sample rate and receive XYZ and motion notifications."""

import argparse
import struct
import time

from bluepy.btle import BTLEDisconnectError, DefaultDelegate, Peripheral, Scanner, UUID

SERVICE_UUID = UUID("a4c00000-7a67-4b5a-9f5b-c3c893bdae40")
ACCEL_UUID = UUID("a4c00001-7a67-4b5a-9f5b-c3c893bdae40")
RATE_UUID = UUID("a4c00002-7a67-4b5a-9f5b-c3c893bdae40")
MOTION_UUID = UUID("a4c00003-7a67-4b5a-9f5b-c3c893bdae40")
CCCD_UUID = UUID(0x2902)


class SensorDelegate(DefaultDelegate):
    def __init__(self, xyz_handle, motion_handle):
        super().__init__()
        self.xyz_handle = xyz_handle
        self.motion_handle = motion_handle
        self.received = 0
        self.motion_received = 0

    def handleNotification(self, handle, data):
        if handle == self.motion_handle:
            if len(data) != 1:
                print(f"Unexpected motion packet: {data.hex(' ')}")
                return
            self.motion_received += 1
            print(f"*** SIGNIFICANT MOTION #{data[0]} detected by STM32"
                  f" (notification {self.motion_received}) ***", flush=True)
            return
        if handle != self.xyz_handle:
            return
        if len(data) != 6:
            print(f"Unexpected XYZ packet: {data.hex(' ')}")
            return
        x, y, z = struct.unpack("<hhh", data)
        self.received += 1
        print(f"#{self.received:03d}  X={x:6d} mg  Y={y:6d} mg  Z={z:6d} mg"
              f"  [hex: {data.hex(' ')}]", flush=True)


def find_board(seconds):
    print(f"Scanning for BlueNRG for {seconds:g} seconds...")
    matches = []
    for device in Scanner().scan(seconds):
        names = [value for kind, _, value in device.getScanData()
                 if kind in (0x08, 0x09)]
        if "BlueNRG" in names:
            matches.append(device)
    if not matches:
        raise RuntimeError("BlueNRG was not found. Disconnect LightBlue and check advertising.")
    matches.sort(key=lambda device: device.rssi, reverse=True)
    for index, device in enumerate(matches):
        print(f"  {index}: {device.addr} ({device.addrType}), RSSI {device.rssi} dBm")
    if len(matches) == 1:
        return matches[0].addr, matches[0].addrType
    index = int(input("Which BlueNRG is your board? Enter its number: "))
    if index not in range(len(matches)):
        raise ValueError("Device number is outside the list")
    return matches[index].addr, matches[index].addrType


def characteristic(service, uuid):
    matches = service.getCharacteristics(forUUID=uuid)
    if not matches:
        raise RuntimeError(f"Characteristic {uuid} is missing from the Lab4 service")
    return matches[0]


def connect_board(address, address_type):
    for attempt in range(1, 4):
        print(f"Connecting to {address} ({address_type}), attempt {attempt}/3...")
        try:
            return Peripheral(address, addrType=address_type)
        except BTLEDisconnectError:
            if attempt == 3:
                raise
            print("BLE connection did not establish; retrying in 2 seconds...")
            time.sleep(2)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rate", type=int, default=5,
                        help="Requested notification rate in Hz, 1–20 (default: 5)")
    parser.add_argument("--duration", type=float, default=30,
                        help="Seconds to receive notifications (default: 30)")
    parser.add_argument("--scan", type=float, default=10,
                        help="Seconds to scan when no address is given (default: 10)")
    parser.add_argument("--address", help="Board's current BLE address; skips the scan")
    args = parser.parse_args()
    if not 1 <= args.rate <= 20:
        parser.error("--rate must be 1–20 Hz")
    if args.duration <= 0 or args.scan <= 0:
        parser.error("--duration and --scan must be positive")

    address, address_type = ((args.address, "random") if args.address
                             else find_board(args.scan))
    peripheral = connect_board(address, address_type)
    try:
        service = peripheral.getServiceByUUID(SERVICE_UUID)
        accel = characteristic(service, ACCEL_UUID)
        rate = characteristic(service, RATE_UUID)
        motion = characteristic(service, MOTION_UUID)
        descriptors = accel.getDescriptors(forUUID=CCCD_UUID)
        if not descriptors:
            raise RuntimeError("XYZ has no CCCD (0x2902) for notifications")
        cccd = descriptors[0]
        motion_descriptors = motion.getDescriptors(forUUID=CCCD_UUID)
        if not motion_descriptors:
            raise RuntimeError("Motion has no CCCD (0x2902) for notifications")
        motion_cccd = motion_descriptors[0]

        print(f"Connected to Lab4 service {SERVICE_UUID}")
        print(f"Initial frequency: {struct.unpack('<H', rate.read())[0]} Hz")
        print(f"Initial motion event counter: {motion.read()[0]}")
        delegate = SensorDelegate(accel.getHandle(), motion.getHandle())
        peripheral.setDelegate(delegate)
        cccd.write(b"\x01\x00", withResponse=True)  # Enable notifications.
        motion_cccd.write(b"\x01\x00", withResponse=True)
        cccd_value = cccd.read()
        print(f"CCCD readback: {cccd_value.hex(' ')} (01 00 means Notify)")
        if cccd_value != b"\x01\x00":
            raise RuntimeError("Notifications were not enabled")
        motion_cccd_value = motion_cccd.read()
        print(f"Motion CCCD readback: {motion_cccd_value.hex(' ')}")
        if motion_cccd_value != b"\x01\x00":
            raise RuntimeError("Motion notifications were not enabled")

        requested = struct.pack("<H", args.rate)
        rate.write(requested, withResponse=True)
        readback = rate.read()
        print(f"Wrote rate: {requested.hex(' ')}; readback: {readback.hex(' ')}")
        if readback != requested:
            raise RuntimeError("Rate readback does not match the requested value")

        print(f"Waiting {args.duration:g} seconds for XYZ and motion notifications."
              " Walk several steps with the board.")
        deadline = time.monotonic() + args.duration
        while (remaining := deadline - time.monotonic()) > 0:
            peripheral.waitForNotifications(min(1.0, remaining))
        print(f"Received {delegate.received} XYZ notifications")
        print(f"Received {delegate.motion_received} motion notifications")
        if delegate.received == 0:
            raise RuntimeError("No XYZ notifications arrived; check the STM32 BLE connection")
    finally:
        peripheral.disconnect()


if __name__ == "__main__":
    main()
