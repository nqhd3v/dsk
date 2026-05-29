# Mosquitto setup

## Generate password file (required before first run)

```bash
cd infrastructure

# Create passwd file with server + dev node accounts
docker run --rm -v "$(pwd)/mosquitto:/mosquitto/config" \
  eclipse-mosquitto:2 sh -c \
  "mosquitto_passwd -c -b /mosquitto/config/passwd dg_server server_dev_pass && \
   mosquitto_passwd    -b /mosquitto/config/passwd node1 node1_dev_pass"
```

## Adding a new ESP node

```bash
# Add credentials
docker exec dg-mosquitto mosquitto_passwd -b /mosquitto/config/passwd node2 node2_pass

# Add ACL block to acl file, then reload:
docker exec dg-mosquitto kill -HUP 1
```
