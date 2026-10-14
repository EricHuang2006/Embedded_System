from bluepy.btle import Peripheral, UUID
from bluepy.btle import Scanner, DefaultDelegate
import time

print ("Connecting...")
#
# dev = Peripheral("47:a2:b0:cf:25:82", 'random')
dev = Peripheral("60:61:71:2c:c5:91", 'random')
print ("Services...")
for svc in dev.services:
    print (str(svc))
#
while(1):
    testService = dev.getServiceByUUID(UUID(0x1111))
    for ch in testService.getCharacteristics():
        print (str(ch))
#
    ch = dev.getCharacteristics(uuid=UUID(0x2222))[0]
    if (ch.supportsRead()):
        print (ch.read())
        print("to hex:", ch.read().hex())

    cccd = ch.getDescriptors(forUUID=UUID(0x2902))[0]
    cccd.write(b"\x02\x00", withResponse=True)
    print("CCCD readback:", cccd.read().hex(" "))
    time.sleep(1)
#
dev.disconnect()
