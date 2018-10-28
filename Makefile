KERN_DIR = /home/cfinvo/lzf/5-android_learn/3288/kernel

all:
	make -C $(KERN_DIR) M=`pwd` modules 

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order

obj-m	+= zf_input_3288.o
