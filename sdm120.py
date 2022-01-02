#!/usr/bin/env python3
import serial
import struct
import sys

class ModBus():
    devaddr=1
    READCMD=4
    def __init__(self,addr):
        self.devaddr=addr
    def gen_read_frame(self,cmd):
        frame=struct.pack('>BBHH',self.devaddr,self.READCMD,cmd.regaddr,cmd.count)
        frame+=(struct.pack('>H',self.modbus_crc(frame)))
        return frame


    @staticmethod
    def modbus_crc(buf):
        crc=0xffff
        for b in buf:
            crc ^= b # XOR byte into least sig. byte of crc
            for i in range(8):
                if (crc & 1) !=0:
                    crc >>=1
                    crc ^= 0xA001
                else:
                    crc >>= 1
        # Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
        crc=crc.to_bytes(2,'little')
        crc=int.from_bytes(crc,'big')
        return crc
    


class Sdm120():
    debug=0
    class Sdm120Register():
        def __init__(self,regaddr,name,unit,skip,count=2):
            self.regaddr=regaddr
            self.name=name
            self.unit=unit
            self.skip=skip
            self.count=count
    REGLIST=list()
    REGLIST.append(Sdm120Register(0x0000,"voltage","V",0))
    REGLIST.append(Sdm120Register(0x0006,"current","A",1))
    REGLIST.append(Sdm120Register(0x000c,"power","W",0))
    REGLIST.append(Sdm120Register(0x0012,"aapower","VAr",1))
    REGLIST.append(Sdm120Register(0x0018,"rapower","VAr",1))
    REGLIST.append(Sdm120Register(0x001e,"pfactor","",1))
    REGLIST.append(Sdm120Register(0x0046,"frequency","Hz",0))
    REGLIST.append(Sdm120Register(0x0048,"iaenergy","kWh",0))
    REGLIST.append(Sdm120Register(0x004a,"eaenergy","kWh",0))
    REGLIST.append(Sdm120Register(0x0156,"taenergy","kWh",1))

    def __init__(self,dev,modbusAddr=1,debug=0):
        self.debug=debug
        self.mb=ModBus(modbusAddr)
        self.mb.gen_read_frame(self.REGLIST[0])
        self.s = serial.Serial(port=dev, baudrate = 2400, parity =serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS, timeout=1)
        self.s.close()
        self.s.open()
        self.s.flushOutput()
        self.s.flushInput()

    def __del__(self):
        self.close()

    def close(self):
        self.s.close()

    def read_reg(self,cmd):
        read_frame=self.mb.gen_read_frame(cmd)
        if self.debug: print("TX:%s"%read_frame.hex())
        self.s.write(read_frame)
        data=self.s.read(9)
        [addr,cmd,datalen,value,chksum_rx]=struct.unpack('>BBBfH',data)
        if self.debug: print("RX:%s"%data.hex())
        chksum=self.mb.modbus_crc(data[0:-2])
        if chksum != chksum_rx:
            print("checksum error, recv:0x%x, calc:0x%x"%(chksum_rx,chksum))
        return value
    
    def print_all(self):
        for reg in self.REGLIST:
            print("%10s: %10.3f %s"%(reg.name,self.read_reg(reg),reg.unit))

    def print_sql(self):
        colnames=list()
        values=list()
        for reg in self.REGLIST:
            if reg.skip:continue
            colnames.append("`%s`"%reg.name)
            values.append('%f'%self.read_reg(reg))
        print('(%s)\nVALUES\n(%s);\n'%(",".join(colnames),",".join(values)))

if __name__ == "__main__":
    debug=0
    serialDev='/dev/modbus_dongle'
    sqlmode=False
    for arg in sys.argv[1:]:
        if arg =="-v":debug=1
        elif arg=="-sql":sqlmode=True
        else:
            serialDev=arg

    sdm120=Sdm120(dev=serialDev,debug=debug)
    if sqlmode:sdm120.print_sql()
    else: sdm120.print_all()