# CI Setup for ESP-AMP

Our CI is running on a Orange Pi 5 Plus (4GB version), below is a memo for the setup.

## `/etc/gitlab-runner/config.toml`

```toml
concurrent = 3
check_interval = 0
connection_max_age = "15m0s"
shutdown_timeout = 0

[session_server]
  session_timeout = 1800

[[runners]]
  name = "orangepi5plus"
  url = "https://gitlab.com"
  id = 88888888
  token = "glrt-XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
  token_obtained_at = 2025-02-31T23:59:59Z
  token_expires_at = 0001-01-01T00:00:00Z
  executor = "docker"
  [runners.cache]
    MaxUploadedArchiveSize = 0
    [runners.cache.s3]
    [runners.cache.gcs]
    [runners.cache.azure]
  [runners.docker]
    tls_verify = false
    image = "ubuntu:latest"
    pull_policy = ["always", "if-not-present"]
    disable_entrypoint_overwrite = false
    oom_kill_disable = false
    disable_cache = false
    volumes = ["/cache"]
    shm_size = 0
    network_mtu = 0
    memory = "1.5g"
    memory_swap = "3.0g"
    cpus = "2.5"
    devices = ["/dev/ttyUSB0", "/dev/ttyUSB1"]
```

> **NOTE**
> 
> You has to install the gitlab runner with sudo.

## Cron job for cleaning unused image

Result of `sudo crontab -e`

```
  1 # Edit this file to introduce tasks to be run by cron.
  2 #
  3 # Each task to run has to be defined through a single line
  4 # indicating with different fields when the task will be run
  5 # and what command to run for the task
  6 #
  7 # To define the time you can provide concrete values for
  8 # minute (m), hour (h), day of month (dom), month (mon),
  9 # and day of week (dow) or use '*' in these fields (for 'any').
 10 #
 11 # Notice that tasks will be started based on the cron's system
 12 # daemon's notion of time and timezones.
 13 #
 14 # Output of the crontab jobs (including errors) is sent through
 15 # email to the user the crontab file belongs to (unless redirected).
 16 #
 17 # For example, you can run a backup of all your user accounts
 18 # at 5 a.m every week with:
 19 # 0 5 * * 1 tar -zcf /var/backups/home.tgz /home/
 20 #
 21 # For more information see the manual pages of crontab(5) and cron(8)
 22 #
 23 # m h  dom mon dow   command
 24 0 12 * * * /usr/bin/yes | /usr/bin/docker image prune
```
