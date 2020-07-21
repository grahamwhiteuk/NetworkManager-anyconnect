# NetworkManager-anyconnect

NetworkManager-anyconnect is a VPN plugin for NetworkManager.  It extends NetworkManager in a similar way to [other VPN plugins](https://wiki.gnome.org/Projects/NetworkManager/VPN) and provides a wrapper to the proprietary Cisco AnyConnect VPN client.

## Usage

Since NetworkManager-anyconnect provides a wrapper around the AnyConnect client, you should ensure that you can first connect via AnyConnect.  Set up your certificates and connection using the proprietary client and test your connection.  Once successfully set up and with NetworkManager-anyconnect installed on your system, you should see a `Cisco AnyConnect` option when adding a new VPN connection.  Select this option to configure your VPN and you will be presented with the configuration screen (shown below).  Configuration is very simple, since you have your certificates set up already you just need to choose the VPN gateway to connect to from the drop down list and give your VPN connection a name.

![AnyConnect Configuration](https://user-images.githubusercontent.com/1632332/86220337-5e8ea680-bb7b-11ea-93c7-a95dfa3340f1.png "AnyConnect Configuration")

### Known Issue and Work Around

Whether you hit this problem will depend on two things being true:
1. Your AnyConnect gateway needs to be configured to require downloading data to VPN clients
2. Your NetworkManager must be started by systemd using `ProtectHome=read-only`

As outlined in the [AnyConnect Manual](https://www.cisco.com/c/en/us/td/docs/security/vpn_client/anyconnect/anyconnect48/administration/guide/b_AnyConnect_Administrator_Guide_4-8.pdf), certificates must live in the `~/.cisco/` directory.  Similarly, AnyConnect may download data from the VPN server during the connection attempt and if so, this downloaded data is also written to the `~/.cisco/` directory.  However, in many Linux distributions the NetworkManager service is started with read-only access to the entire `/home` directory structure using the `ProtectHome=read-only` [configuration from systemd](https://www.freedesktop.org/software/systemd/man/systemd.exec.html).  In these cases where AnyConnect needs to write to `~/.cisco/` but cannot, the connection attempt will fail.

The work-around for this issue is to move your certificates elsewhere within your home directory and create a `~/.cisco` directory in another branch of the file system that is writeable to your user id.  This can be achieved using the `/var/tmp/` directory using the following commands that should be run under your own user ID (not root) as an example:

```shell
mv ~/.cisco ~/.cisco2
mkdir /var/tmp/${USER}-cisco
ln -s /var/tmp/${USER}-cisco ~/.cisco
ln -s ~/.cisco2/certificates /var/tmp/${USER}-cisco
```

The above commands:
1. move your `~/.cisco/` directory out of the way so it can be replaced with a symbolic link
2. create a new directory that will be writeable to NetworkManager sub-processes
3. link the writeable directory from your `~/.cisco/` directory
4. keep your certificates in your home directory and link those to the writeable directory (they will remain read-only)

The `/var/tmp/` directory is recommended for this work-around since it is considered more persistent than `/tmp/`.  However, you may find that the sub-directory you created there is wiped out from time to time so you may want to set something up perhaps with a systemd timer or cron to make sure the directory remains in place for when you need it for your VPN connections.

## Building

You will need gcc and the GNU autotools installed on your build system.  There is a `autogen.sh` script you can use to generate the required build files.  Then it's the standard configure and make script you'd expect.  The `autogen.sh` script is set to run the configure script but you may choose to reconfigure the build.  An example is shown below:

```shell
cd NetworkManager-anyconnect
./autogen.sh
./configure --prefix=/usr --exec-prefix=/usr --disable-static --enable-more-warnings=yes
make -j
sudo make install
```
