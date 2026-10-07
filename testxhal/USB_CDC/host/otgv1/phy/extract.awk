# Compile the real platform helpers verbatim, not a rewritten PHY model.
/^(bool|void) stm32_otg2_phy_(start|stop)\(void\) \{/ { copy = 1; found++ }
copy { print }
copy && /^}/ { copy = 0; print "" }
END {
  if (found != 2) {
    print "Missing or duplicate U5 PHY helpers" > "/dev/stderr"
    exit 1
  }
}
