#!/bin/sh
set -e

BASE_DIR="${PWD}"
PLATFORM=$(uname -s)

if [ ${PLATFORM} == Darwin ]; then

    echo "Checking for brew"
    if ! brew --version ; then
        echo "Error: brew not found"
        exit 1
    fi

    if ! brew ls --versions glfw3 > /dev/null ; then
        echo "Installing GLFW"
        brew install glfw3
    fi

    GLFW_DIR=$(brew --prefix glfw3)
    echo "GLFW installed at ${GLFW_DIR}"

    mkdir -p external/include
    mkdir -p external/lib

    echo "Creating symlinks"
    ln -sfv ${GLFW_DIR}/include/* external/include
    ln -sfv ${GLFW_DIR}/lib/* external/lib

else

    GLFW_DIR="${PWD}/glfw_latest"

    if [ ! -d "${GLFW_DIR}" ]; then

        echo "Fetching source of latest GLFW release"
        curl -o "${GLFW_DIR}.tar.gz" -L "$( curl -s "https://api.github.com/repos/glfw/glfw/releases/latest" | \
                                            grep "tarball_url*"                                              | \
                                            tr -d '[:space:]'                                                | \
                                            cut -d : -f 2,3                                                  | \
                                            tr -d \"                                                         | \
                                            tr -d ,                                                            )"

        echo "Unpacking GLFW tarball"
        mkdir "${GLFW_DIR}"
        tar -xf "${GLFW_DIR}.tar.gz" -C "${GLFW_DIR}" --strip-components=1
        rm "${GLFW_DIR}.tar.gz"

    else
        echo "GLFW folder already exists"
    fi

    echo "Checking for cmake"
    if ! cmake --version ; then
        echo "Error: cmake not found"
        exit 1
    fi

    echo "Running cmake"
    cd "${GLFW_DIR}"
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX="${BASE_DIR}/external" .

    echo "Installing GLFW"
    make install

    cd -
    rm -r "${GLFW_DIR}"

fi

echo "Done"
