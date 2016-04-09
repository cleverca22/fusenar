{ writeScriptBin, container_data, fusenar, systemd, tiny_container }:

writeScriptBin "start_container" ''
#!/bin/sh

mkdir -pv /mnt/nix/store/

${fusenar}/bin/fusenar -o allow_other -d -s -f /mnt/cache/ ${container_data} /mnt/nix/store/ > /mnt/fusenar.stdout 2>/mnt/fusenar.stderr &
FUSEPID=$!

sleep 5

EXIT_ON_REBOOT=1 NOTIFY_SOCKET= ${systemd}/bin/systemd-nspawn -D /mnt/ -M fusenar ${tiny_container}/init
''
