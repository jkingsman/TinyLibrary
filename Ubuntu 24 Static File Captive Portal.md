Man, I could not find a tutorial for this anywhere. So, how to set up a static file server and captive portal on Ubuntu 24, including on a Raspberry Pi Zero 2W. It serves static files, broadcasts an open wifi network, and uses DNS + DHCP to direct people to a captive portal (both all-DNS method and DHCP option 114).

## Package Installation

First, install the packages we'll definitely need.

```bash
sudo apt update
sudo apt upgrade -y
sudo apt install -y vim dnsmasq hostapd
````

## Caddy

Then let's install Caddy and get it serving files. If you'd like to use an alternative webserver, feel free!

```bash
# caddy install itself
curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/gpg.key' | sudo gpg --dearmor -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg
curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/debian.deb.txt' | sudo tee /etc/apt/sources.list.d/caddy-stable.list
sudo apt update
sudo apt install -y caddy

# caddy files dir
sudo mkdir -p /srv/static
sudo chmod -R 777 /srv

# serve from that directory
sudo bash -c "cat > /etc/caddy/Caddyfile"<< EOF
:80 {
    root * /srv/static
    encode gzip
    file_server {
        index index.html
    }
    log {
        output file /var/log/caddy/my-static-site.log
    }

	redir /connecttest.txt /index.html
	redir /wpad.dat /index.html
	redir /generate_204 /index.html
	redir /redirect /index.html
	redir /hotspot-detect.html /index.html
	redir /canonical.html /index.html
	redir /success.txt /index.html
	redir /success.html /index.html
	redir /ncsi.txt /index.html
}
EOF

# boot caddy at startup
sudo systemctl restart caddy
sudo systemctl enable caddy.service
```

Note that the extra redirects in the Caddy config help us with captive portal tests done by mobile devices etc. If you don't want captive portal attempts directed to the index, you can omit those.

## Upload your stuff

Now's a good time to put whatever you want served into the `/src/static` directory.

Something like `rsync -r -v --progress -e ssh --exclude=".[!.]*" . user@192.168.0.144:/srv/static` is handy for me when I'm uploading a git repo of static files to send everything but the dotfiles. Obviously you'll need to set your user and IP.

## Network Config

### Disable Old Stuff

Turn off the stub resolver with

```bash
sudo systemctl disable systemd-resolved
sudo systemctl stop systemd-resolved
```
You can validate that it's dead by running `sudo lsof -i:53` and checking that you get no results (i.e. nothing listening on port 53).

### Configure DNS + DHCP

Place DNS + DHCP config:

```bash
sudo tee /etc/dnsmasq.conf << EOF
bind-dynamic
dhcp-authoritative
no-resolv
domain-needed
bogus-priv
strict-order
expand-hosts
interface=wlan0

# the magic -- all roads lead to 10.0.1.1
address=/#/10.0.1.1

# dhcp config
dhcp-range=10.0.1.2,10.0.1.50,255.255.255.0,28h
dhcp-option-force=114,"http://10.0.1.1"
dhcp-option=option:router,10.0.1.1
dhcp-option=option:dns-server,10.0.1.1
dhcp-option=option:netmask,255.255.255.0
EOF

# undo any weird resolv.conf symlinking and name myself as king
sudo unlink /etc/resolv.conf
echo nameserver 127.0.0.1 | sudo tee /etc/resolv.conf

# let there be dhcp and dns!
sudo systemctl unmask dnsmasq
sudo systemctl enable dnsmasq
sudo systemctl restart dnsmasq
```

### Disable CloundInit

If you did this via RasPi imager, you likely have some stale network configs hanging around. Let's flush those out.

```bash
echo "network: {config: disabled}" | sudo tee /etc/cloud/cloud.cfg.d/99-disable-network-config.cfg
sudo touch /etc/cloud/cloud-init.disabled
sudo rm -rf /etc/netplan/50-cloud-init.yaml
sudo netplan generate
```

### Set up HostAPD

```bash
sudo tee /etc/hostapd/hostapd.conf << EOF
interface=wlan0
driver=nl80211
ssid=FreeEbooksConnectToRead
hw_mode=g
channel=6
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
EOF

sudo tee /etc/default/hostapd  << EOF
DAEMON_CONF="/etc/hostapd/hostapd.conf"
EOF
```

### Set Static IP

```bash
# set static IP
sudo mkdir -p /etc/systemd/network
sudo tee /etc/systemd/network/01-static-ip.network << EOF
[Match]
Name=wlan0

[Network]
Address=10.0.1.1/32
Gateway=10.0.1.1
DNS=10.0.1.1
EOF
```

### Restart!


```bash
# bounce services
sudo systemctl unmask systemd-networkd
sudo systemctl enable systemd-networkd
sudo systemctl start systemd-networkd

# let there be wifi
sudo systemctl unmask hostapd
sudo systemctl enable hostapd
sudo systemctl start hostapd

# to flush any historical wifi config out
sudo reboot
```

And now you should be able to connect to your new AP after reboot!

## All in One script

```bash
# Take 3

curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/gpg.key' | sudo gpg --dearmor -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg
curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/debian.deb.txt' | sudo tee /etc/apt/sources.list.d/caddy-stable.list
sudo apt update
sudo apt upgrade -y
sudo apt install -y vim dnsmasq hostapd caddy

# set up caddy serving statically
sudo mkdir -p /srv/static
sudo chmod -R 777 /srv

sudo bash -c "cat > /etc/caddy/Caddyfile"<< EOF
:80 {
    root * /srv/static
    encode gzip
    file_server {
        index index.html
    }
    log {
        output file /var/log/caddy/my-static-site.log
    }

	redir /connecttest.txt /index.html
	redir /wpad.dat /index.html
	redir /generate_204 /index.html
	redir /redirect /index.html
	redir /hotspot-detect.html /index.html
	redir /canonical.html /index.html
	redir /success.txt /index.html
	redir /success.html /index.html
	redir /ncsi.txt /index.html
}
EOF

# boot caddy at startup
sudo systemctl restart caddy
sudo systemctl enable caddy

# turn off stub resolver
sudo systemctl disable systemd-resolved
sudo systemctl stop systemd-resolved

# can validate that it's dead by running
# 	sudo lsof -i:53
# and getting  back nothing

# place dns + dhcp config
sudo tee /etc/dnsmasq.conf << EOF
bind-dynamic
dhcp-authoritative
no-resolv
domain-needed
bogus-priv
strict-order
expand-hosts
interface=wlan0

# the magic -- all roads lead to 10.0.1.1
address=/#/10.0.1.1

# dhcp config
dhcp-range=10.0.1.2,10.0.1.50,255.255.255.0,28h
dhcp-option-force=114,"http://10.0.1.1"
dhcp-option=option:router,10.0.1.1
dhcp-option=option:dns-server,10.0.1.1
dhcp-option=option:netmask,255.255.255.0
EOF

# undo any weird resolv.conf symlinking and name myself as king
sudo unlink /etc/resolv.conf
echo nameserver 127.0.0.1 | sudo tee /etc/resolv.conf

# let there be dhcp and dns!
sudo systemctl unmask dnsmasq
sudo systemctl enable dnsmasq
sudo systemctl restart dnsmasq

# validate resolution with
# 	dig @localhost example.com
# should return 10.0.1.1

# config network
echo "network: {config: disabled}" | sudo tee /etc/cloud/cloud.cfg.d/99-disable-network-config.cfg
sudo touch /etc/cloud/cloud-init.disabled
sudo rm -rf /etc/netplan/50-cloud-init.yaml
sudo netplan generate

sudo tee /etc/hostapd/hostapd.conf << EOF
interface=wlan0
driver=nl80211
ssid=FreeEbooksConnectToRead
hw_mode=g
channel=6
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
EOF

sudo tee /etc/default/hostapd  << EOF
DAEMON_CONF="/etc/hostapd/hostapd.conf"
EOF

# set static IP
sudo mkdir -p /etc/systemd/network
sudo tee /etc/systemd/network/01-static-ip.network << EOF
[Match]
Name=wlan0

[Network]
Address=10.0.1.1/32
Gateway=10.0.1.1
DNS=10.0.1.1
EOF

# bounce services
sudo systemctl unmask systemd-networkd
sudo systemctl enable systemd-networkd
sudo systemctl start systemd-networkd

# let there be wifi
sudo systemctl unmask hostapd
sudo systemctl enable hostapd
sudo systemctl start hostapd

sudo reboot # to flush any historical wifi config out

# connect to your network!

# transfer static bundles -- LOCAL COMMAND
# cd <static file directory>
# rsync -r -v --progress -e ssh --exclude=".[!.]*" . jack@10.0.1.1:/srv/static
```
