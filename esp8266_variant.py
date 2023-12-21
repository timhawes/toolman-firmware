Import("env")
env.Append(CPPDEFINES=[
        ("ARDUINO_VARIANT", '\\"%s\\"' % env.BoardConfig().get("build.variant").replace('"', ""))
    ]
)
