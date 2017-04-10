#!/system/bin/sh
usb_config=`getprop persist.sys.usb.config`
       if [ -e /data/property/persist.sys.usb.config ]; then
	    echo donothing
	else
	     setprop persist.sys.usb.config $usb_config
	fi


