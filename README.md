# PostgreSQL Configuration Guide
This document provides step-by-step instructions for setting up PostgreSQL on a host machine and running AuthServer in a Docker container with proper database connectivity.
## PostgreSQL Setup on Host Machine
For the application to work correctly, PostgreSQL must be accessible from your Docker container. Execute the following commands on your host machine:
### 1. Configure PostgreSQL for External Connections
``` bash
apt install -y postgresql-client
```

``` bash
# Edit the pg_hba.conf file to allow external connections
sudo vim /etc/postgresql/15/main/pg_hba.conf
```
Add the following line to permit connections from any IP address:
``` 
# Allow connections from all addresses (use 'md5' instead of 'trust' in production)
host    all             all             0.0.0.0/0            trust
```

``` bash
# Edit postgresql.conf to listen on all network interfaces
sudo vim /etc/postgresql/15/main/postgresql.conf
```
Find and uncomment or modify the line:
``` 
listen_addresses = '*'
```
### 2. Restart PostgreSQL and Check Status
``` bash
sudo systemctl restart postgresql.service
sudo systemctl status postgresql.service
```
### 3. Configure PostgreSQL User
``` bash
# Connect to PostgreSQL
psql -U postgres

# In the psql console, set a password for the postgres user
ALTER USER postgres WITH PASSWORD '123456';

# Exit psql
\q
```
### 4. Install iproute2 and Get Host IP Address
``` bash
sudo apt install iproute2

# Get the host IP address for use in Docker
ip route | grep default | awk '{print $3}'
```
### 5. Verify PostgreSQL Connection Using the Retrieved IP Address
``` bash
# Test connection to PostgreSQL
psql "postgresql://postgres:123456@$(ip route | grep default | awk '{print $3}'):5432/postgres" -c "SELECT version();"

# Create the database for AuthServer
psql "postgresql://postgres:123456@$(ip route | grep default | awk '{print $3}'):5432/postgres" -c "CREATE DATABASE users_db;"

# Import the database schema
psql "postgresql://postgres:123456@$(ip route | grep default | awk '{print $3}'):5432/users_db" -f dbusers.sql
```
## Running AuthServer in Docker
After configuring PostgreSQL, you can launch AuthServer in a container:
### 1. Run Docker Container
``` bash
# Replace 10.10.0.1 with your host machine's IP address obtained earlier
docker run -it --rm \
  -e DB_CONNECTION_STRING="postgresql://postgres:123456@10.10.0.1:5432/users_db" \
  -p 60000:60000 \
  -v $(pwd):/app \
  ubuntu25 bash
```
### 2. Build SrvCore and AuthServer Inside the Container
``` bash
# Build the SrvCore library
cd app/
rm -r build/
mkdir build && cd build && cmake .. && cmake --build . && cmake --install .
```
