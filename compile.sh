# Validates the existance of the TPE-ARQ container, starts it up & compiles the project
CONTAINER_NAME="TPE-ARQ-g08-64018-64288-64646"

# COLORS
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m'

# Build mode: default K&R, optional "buddy"
MM_ARG=""
if [ "$1" = "buddy" ]; then
    MM_ARG="MM_IMPL=buddy"
    echo "${GREEN}Using buddy memory manager (MM_IMPL=buddy).${NC}"
else
    echo "${GREEN}Using default memory manager (K&R).${NC}"
fi

docker ps -a &> /dev/null

if [ $? -ne 0 ]; then
    echo "${RED}Docker is not running. Please start Docker and try again.${NC}"
    exit 1
fi

# Check if container exists
if [ ! "$(docker ps -a | grep "$CONTAINER_NAME")" ]; then
    echo "${YELLOW}Container $CONTAINER_NAME does not exist. ${NC}"
    echo "Pulling image..."
    docker pull agodio/itba-so:2.0
    echo "Creating container..."
    # Note: ${PWD}:/root. Using another container to compile might fail as the compiled files would not be guaranteed to be at $PWD
    # Always use TPE-ARQ to compile
    docker run -d -v ${PWD}:/root --security-opt seccomp:unconfined -it --name "$CONTAINER_NAME" agodio/itba-so:2.0
    echo "${GREEN}Container $CONTAINER_NAME created.${NC}"
else
    echo "${GREEN}Container $CONTAINER_NAME exists.${NC}"
fi

# Start container
docker start "$CONTAINER_NAME" &> /dev/null
echo "${GREEN}Container $CONTAINER_NAME started.${NC}"

# Choose exec flags: use -it when attached to a TTY, otherwise -i
if [ -t 0 ]; then
    DOCKER_EXEC_FLAGS="-it"
else
    DOCKER_EXEC_FLAGS="-i"
fi

# Compiles
docker exec $DOCKER_EXEC_FLAGS "$CONTAINER_NAME" make clean -C /root/ $MM_ARG && \
docker exec $DOCKER_EXEC_FLAGS "$CONTAINER_NAME" make all -C /root/Toolchain $MM_ARG && \
docker exec $DOCKER_EXEC_FLAGS "$CONTAINER_NAME" make all -C /root/ $MM_ARG


if [ $? -ne 0 ]; then
    echo "${RED}Compilation failed.${NC}"
    exit 1
fi

echo "${GREEN}Compilation finished.${NC}"
