{
   'targets': [{
        'target_name': 'freeling',
        "sources": [ "src/init.cpp" ],
        'cflags': [
            '<!@(pkg-config --cflags freeling)','-fpermissive'
        ],
        'cflags_cc!': [ '-fno-exceptions'],
        'ldflags': [
            '<!@(pkg-config --libs-only-L --libs-only-other freeling)'
        ],
        'libraries': [
            '<!@(pkg-config --libs-only-l freeling)'
        ]
    }]
}