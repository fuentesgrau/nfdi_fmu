docker run --network=host -v ./script.lua:/script.lua -v ./results:/results -v ./lua_test.conf:/config.conf fmu_villas node /config.conf
