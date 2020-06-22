from subprocess import PIPE, Popen

transmit_delay_dict = {}
size = 5

nameList = ['50b', '1', '5', '50', '500', '5000']

for j in range(0,6):
    
    print(nameList[j])
    in_pwd = "../data/"
    out_pwd = "../data/"

    in_pwd = in_pwd + str(nameList[j]) +".in"
    out_pwd += out_pwd + str(nameList[j]) +".out"
    transmit_delay_dict[nameList[j]] = {}
    transmit_delay_dict[nameList[j]]["m"] = []
    transmit_delay_dict[nameList[j]]["s"] = []

    for i in range(20):
        print(i,end = ' ')
        m = Popen(['./master', '1', in_pwd, 'mmap'], stdout = PIPE, stderr = PIPE)
        s = Popen(['./slave', '1', out_pwd, 'mmap', '127.0.0.1'], stdout = PIPE, stderr = PIPE)

        outm, error = m.communicate()
        outs, error = s.communicate()
        outm = outm.decode('utf-8')
        outs = outs.decode('utf-8')
        print(outm)
        a = outm.find(":")
        b = outm.find("ms")
        print(outm[a+1:b-1])
        transmit_delay_dict[nameList[j]]["m"].append(float(outm[a+1:b-1]))
        outm = ""

        a = outs.find(":")
        b = outs.find("ms")
        print(outs[a+1:b-1])
        transmit_delay_dict[nameList[j]]["s"].append(float(outs[a+1:b-1]))
        outs = ""


    
print(transmit_delay_dict)

file = open("resultMtoM.txt",'w')
for key in transmit_delay_dict:
    print(key)
    file.write(str(key) + '\n')
    for item in transmit_delay_dict[key]["m"]:
        print(item)
        file.write(str(item))
        file.write(',')
    file.write('\n')
    for item in transmit_delay_dict[key]["s"]:
        print(item)
        file.write(str(item))
        file.write(',')
    file.write('\n')
