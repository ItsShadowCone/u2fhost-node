{
    "targets": [
        {
            "target_name": "u2fhostnode",
            "sources": [
                "src/index.cc"
            ],
            "include_dirs": [
                "thirdparty/inc"
            ],
            "libraries": [
                "../thirdparty/lib/<(OS)/<!@(node -p 'process.arch')/libu2f-host.a",
                "../thirdparty/lib/<(OS)/<!@(node -p 'process.arch')/libjson-c.a",
                "../thirdparty/lib/<(OS)/<!@(node -p 'process.arch')/libhidapi-hidraw.a",
                "../thirdparty/lib/<(OS)/<!@(node -p 'process.arch')/libusb-1.0.a"
            ],
            "conditions": [
                ["OS=='linux'", {
                    "libraries": [
                        "-ludev"
                    ]
                }, { #else

                }]
            ]
        }
    ]
}
