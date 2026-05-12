-- Install with: luarocks install lunajson luasockets

-- SPDX-FileCopyrightText: 2014-2023 Institute for Automation of Complex Power Systems, RWTH Aachen University
-- SPDX-License-Identifier: Apache-2.0

function process(smp)
	info("Process Lua Hook")

  smp.data.pulse1 = smp.data.pulse1 * 2

  return 0
end


