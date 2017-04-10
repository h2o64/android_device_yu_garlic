#!/system/bin/sh
#BEGIN<FFBAKKPKA-239><20150202><Factory reset,USB -> set to the MTP as default,and USB Debug set default on>
	rm -r /persist/Tlog*
	usb_config=`getprop persist.sys.usb.config`
#tinno_sig=`getprop persist.sys.tinno.sig`

#case "$tinno_sig" in
#	"" | "none")
#	setprop persist.sys.tinno.sig OK
#	;;

#	*)
		case "$usb_config" in
		    "" | "none" | "adb")
			if [ -e /persist/factoryreset ]; then
				   mkdir -p /persist/Tlogclosediag
				   setprop persist.sys.usb.config mtp
			else
				mkdir -p /persist/Tlogopendiag
				setprop persist.sys.usb.config diag,serial_smd,rmnet_bam,adb
				mkdir -p /persist/factoryreset
			fi
		   ;;
		   *)
		  mkdir -p /persist/TlogoDonothing
		   ;;
	        esac
#      ;;
#esac
#END<FFBAKKPKA-239><20150202><Factory reset,USB -> set to the MTP as default,and USB Debug set default on>

