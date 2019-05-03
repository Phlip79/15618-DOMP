import sys
import random

output_data = open(sys.argv[1], "w+")
output_label = open(sys.argv[2], "w+")
length = int(sys.argv[3])

output_data.write(str(length) + "\n")
output_label.write(str(length) + "\n")

for i in range(length):
	for j in range(6):
		if (j == 5):
			output_data.write(str(random.randint(0,1)) + "\n")
		else:
			output_data.write(str(random.randint(0,1)) + ",")

	for j in range(2):
		if (j == 1):
			output_label.write(str(random.randint(0,1)) + "\n")
		else:
			output_label.write(str(random.randint(0,1)) + ",")


