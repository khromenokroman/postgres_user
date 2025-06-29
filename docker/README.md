# Dockerfile for Development and Building
## Description
This Dockerfile creates an image based on Ubuntu 25.04 containing all necessary 
tools and dependencies for developing and building the SrvCore library. 
The image provides a ready-to-use development environment with installed compilers, 
libraries, and debugging tools.
## Usage
### Building the Image
``` bash
cd docker
sudo docker build --no-cache -t ubuntu25 .
```
### Running the Container
To start a container with the SrvCore project:
``` bash
cd ..
docker run -it --rm -v $(pwd):/app ubuntu25 bash
```
### Building the Project Inside the Container
``` bash
cd /app
mkdir -p build && cd build
cmake ..
make
```
