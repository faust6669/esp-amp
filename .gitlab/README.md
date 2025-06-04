# CI Setup for ESP-AMP

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
    image = "debian:bookworm-slim"
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
    devices = ["/dev/ttyUSB0", "/dev/ttyUSB1"] # or set privileged to true
```

> **NOTE**
> 
> You has to install the gitlab runner with sudo.

## Unused image clean-up

Check out [doucuum](https://github.com/stepchowfun/docuum).
