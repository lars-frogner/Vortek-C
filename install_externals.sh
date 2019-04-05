#!/bin/bash
set -e

BASE_DIR="${PWD}"
PLATFORM=$(uname -s)

if [ ${PLATFORM} == Darwin ]; then

    echo "Checking for brew"
    if ! brew --version > /dev/null 2>&1 ; then
        echo "Installation of brew required"
        echo "This will run the command \"ruby -e \"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)\"\""
        read -p "Do you want to continue? [y/N] " -r
        if [[ $REPLY =~ ^[Yy]$ ]]
        then
            echo "Installing brew"
            ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
        else
            echo "Aborted"
            exit 1
        fi
    fi

    echo "Checking for glfw3"
    if ! brew ls --versions glfw3 > /dev/null 2>&1 ; then

        echo "Installation of glfw3 required"
        echo "This will run the command \"brew install glfw3\""
        read -p "Do you want to continue? [y/N] " -r
        if [[ $REPLY =~ ^[Yy]$ ]]
        then
            echo "Installing glfw3"
            brew install glfw3
        else
            echo "Aborted"
            exit 1
        fi

    fi

    GLFW_DIR=$(brew --prefix glfw3)
    echo "GLFW installed at ${GLFW_DIR}"

    mkdir -p external/include
    mkdir -p external/lib

    echo "Creating symlinks"
    ln -sfv ${GLFW_DIR}/include/* external/include
    ln -sfv ${GLFW_DIR}/lib/* external/lib

else

    echo "Checking for curl"
    if ! curl --version > /dev/null 2>&1 ; then
        echo "Installation of curl required"
        echo "This will run the command \"sudo apt install curl\""
        read -p "Do you want to continue? [y/N] " -r
        if [[ $REPLY =~ ^[Yy]$ ]]
        then
            echo "Installing curl"
            sudo apt install curl
        else
            echo "Aborted"
            exit 1
        fi
    fi

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
    if ! cmake --version > /dev/null 2>&1 ; then
        echo "Installation of cmake required"
        echo "This will run the command \"sudo apt install cmake\""
        read -p "Do you want to continue? [y/N] " -r
        if [[ $REPLY =~ ^[Yy]$ ]]
        then
            echo "Installing cmake"
            sudo apt install cmake
        else
            echo "Aborted"
            exit 1
        fi
    fi

    echo "Checking for xorg-dev"
    if ! dpkg -s xorg-dev > /dev/null 2>&1 ; then
        echo "Installation of xorg-dev required"
        echo "This will run the command \"sudo apt install xorg-dev\""
        read -p "Do you want to continue? [y/N] " -r
        if [[ $REPLY =~ ^[Yy]$ ]]
        then
            echo "Installing xorg-dev"
            sudo apt install xorg-dev
        else
            echo "Aborted"
            exit 1
        fi
    fi

    echo "Checking for libglu1-mesa-dev"
    if ! dpkg -s libglu1-mesa-dev > /dev/null 2>&1 ; then
        echo "Installation of libglu1-mesa-dev required"
        echo "This will run the command \"sudo apt install libglu1-mesa-dev\""
        read -p "Do you want to continue? [y/N] " -r
        if [[ $REPLY =~ ^[Yy]$ ]]
        then
            echo "Installing libglu1-mesa-dev"
            sudo apt install libglu1-mesa-dev
        else
            echo "Aborted"
            exit 1
        fi
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
