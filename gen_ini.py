import os

NODE_COUNTS = [50, 100, 150, 200]
PROTOCOLS = ['Aodv', 'Dsdv', 'Dsr', 'Olsr']
SIM_TIME = 600

proto_map = {
    'Aodv': 'Aodv',
    'Dsdv': 'Dsdv',
    'Dsr': 'Dsr',
    'Olsr': 'Olsr'
}

lines = []
lines.append('[General]')
lines.append('network = manet_comparison.ManetChain')
lines.append(f'sim-time-limit = {SIM_TIME}s')
lines.append('')
lines.append('**.configurator.config = xml("<config><interface hosts=\'*\' address=\'10.x.x.x\' netmask=\'255.255.255.0\'/></config>")')
lines.append('**.mobility.constraintAreaMinX = 0m')
lines.append('**.mobility.constraintAreaMinY = 0m')
lines.append('**.mobility.constraintAreaMaxX = 1500m')
lines.append('**.mobility.constraintAreaMaxY = 1500m')
lines.append('**.mobility.speed = uniform(5, 25)mps')
lines.append('**.mobility.waitTime = 0s')
lines.append('')
lines.append('# Traffic setup')
lines.append('**.numUdpApps = 1')
lines.append('**.udpApp[0].typename = "UdpBasicApp"')
lines.append('**.udpApp[0].destPort = 1000')
lines.append('**.udpApp[0].messageLength = 1024B')
lines.append('**.udpApp[0].sendInterval = exponential(1s)')
lines.append('')

for proto in PROTOCOLS:
    for n in NODE_COUNTS:
        lines.append(f'[Config {proto}{n}]')
        lines.append(f'**.routingProtocol = "{proto_map[proto]}"')
        lines.append(f'**.numNodes = {n}')
        # Set up source-destination pairs: choose 5 pairs of nodes
        sources = [0, int(n*0.2), int(n*0.4), int(n*0.6), int(n*0.8)]
        dests = [1, int(n*0.2)+1, int(n*0.4)+1, int(n*0.6)+1, int(n*0.8)+1]
        for i, (src, dst) in enumerate(zip(sources, dests)):
            if src < n and dst < n:
                lines.append(f'**.node[{src}].udpApp[0].destAddresses = "10.0.0.{dst+1}"')
        lines.append('')

with open('omnetpp.ini', 'w') as f:
    f.write('\n'.join(lines))
print("Generated omnetpp.ini")
