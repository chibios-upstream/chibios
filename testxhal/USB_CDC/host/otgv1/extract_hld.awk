# Extract complete generated functions verbatim, not a rewritten HLD model.
BEGIN {
  split("get_hword ep0_resume_waiter_i ep0_signal_reset_i ep0_reply_or_ack ep0_receive_or_status _usb_error_i _usb_reset _usb_wakeup usbConnectBus usbDisconnectBus usbInitEndpointI usbDisableEndpointsI usbStartReceiveI usbStartTransmitI usbReceive usbTransmit usbEp0WaitSetup usbEp0Reply usbEp0Receive usbEp0Acknowledge usb_post_events_i setup_error set_address usbEp0Stall usbEp0HandleStandardRequest __usb_start_impl __usb_stop_impl drvStart drvStop", functions)
  for (i in functions) wanted[functions[i]] = 1
}
/^(static )?(void|msg_t|uint16_t) [a-zA-Z0-9_]+\(/ {
  name = $0
  sub(/\(.*/, "", name)
  sub(/^.* /, "", name)
  copy = name in wanted
  sync = name == "usbReceive" || name == "usbTransmit"
  if (copy && sync) print "#if USB_USE_SYNCHRONIZATION == TRUE"
}
copy { print }
/^static const uint8_t (zero_status|active_status|halted_status)\[\]/ { print }
copy && /^}/ {
  if (sync) print "#endif"
  print ""
  copy = 0
  found[name]++
}
END {
  for (name in wanted) {
    if (found[name] != 1) {
      print "Missing or duplicate HLD function: " name > "/dev/stderr"
      exit 1
    }
  }
}
