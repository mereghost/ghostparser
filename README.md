# Parsing EVTCs



## Header

First 16 bytes of the file. Unpacks as `Z12xh4x`

The first 12 chars are related to arcdps version

The last 4 are an hex string that identifies the boss.



## Agents

Next 2 bytes identify an int16 for the number of agents.