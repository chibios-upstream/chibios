# Extract complete generated functions verbatim, not a rewritten model.
BEGIN {
  split("usbbnd_if_conflict usbbnd_service_conflict usbServiceBind usbServiceUnbind usbBinderBind usbBinderUnbind usbBinderRegisterService usbBinderUnregisterService cdcsvc_set_default_linecoding cdcsvc_endpoint_in_cb cdcsvc_endpoint_out_cb usbCdcServiceStart usbCdcServiceStop __cdcsvc_objinit_impl __cdcsvc_bind_impl __cdcsvc_unbind_impl", functions)
  for (i in functions) wanted[functions[i]] = 1
}
/^(static )?[a-z_]+ \*?[A-Za-z0-9_]+\(/ {
  name = $0
  sub(/\(.*/, "", name)
  sub(/^.*[ *]/, "", name)
  copy = name in wanted
}
copy { print }
copy && /^}/ {
  print ""
  copy = 0
  found[name]++
}
END {
  for (name in wanted) {
    if (found[name] != 1) {
      print "Missing or duplicate generated function: " name > "/dev/stderr"
      exit 1
    }
  }
}
