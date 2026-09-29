export const PRESETS: Record<string, { name: string; tag: string; description: string; code: string }> = {
  newton_sqrt: {
    name: "Newton's Square Root (Deterministic)",
    tag: "Numerical Recurrence",
    description: "Computes sqrt(2) via Newton-Raphson iteration in a 64-byte memory slab.",
    code: `-- --------------------------------------------------
-- LLFPL: Newton-Raphson Method for Sqrt(2.0)
-- --------------------------------------------------
Require(stdlib.LLFPL)

Slab(State, 64)

Identity(Target, 2.0)
Identity(Steps, 15)
Identity(Offset, 0)

Map(InitState, x,
    write_offset(State, Offset, x)
)

Map(GetCurrent, ignored, read_offset(State, Offset))

Map(Refine, ignored,
    write_offset(State, Offset,
        divide(
            plus(GetCurrent(0), divide(Target, GetCurrent(0))),
            TWO
        )
    )
)

-- 1. Seed initial estimate with 1.0
Commit(InitState(ONE))

-- 2. Run bounded iteration loop in arena
Commit(Loop(Steps, Refine))

-- 3. Report converged root and residual error
Commit(GetCurrent(0))
Commit(minus(Square(GetCurrent(0)), Target))
`
  },
  packet_conditioning: {
    name: "Packet Saturation Filter",
    tag: "Signal Processing",
    description: "Clamps signal amplitude within saturation bounds using a 512-byte ring buffer.",
    code: `-- --------------------------------------------------
-- LLFPL: Packet Saturation Filter
-- --------------------------------------------------
Require(stdlib.LLFPL)

Slab(PacketRing, 512)
Identity(Saturation, 100.0)

Map(Condition, value,
    ClampToRange(value, Negate(Saturation), Saturation)
)

-- Populate ring buffer slots
Commit(write_offset(PacketRing, 0, 245.5))
Commit(write_offset(PacketRing, 8, -180.2))
Commit(write_offset(PacketRing, 16, 42.0))
Commit(write_offset(PacketRing, 24, -99.9))

-- Run condition filter
Commit(Condition(read_offset(PacketRing, 0)))
Commit(Condition(read_offset(PacketRing, 8)))
Commit(Condition(read_offset(PacketRing, 16)))
Commit(Condition(read_offset(PacketRing, 24)))
`
  },
  vector_dot: {
    name: "Vector Dot Product (Cache Slabs)",
    tag: "Vector Math",
    description: "Computes 3D vector dot product using memory-mapped slab offsets.",
    code: `-- --------------------------------------------------
-- LLFPL: 3D Vector Dot Product
-- A = [3.0, 4.0, 5.0], B = [2.0, -1.0, 4.0]
-- --------------------------------------------------
Require(stdlib.LLFPL)

Slab(VecA, 64)
Slab(VecB, 64)

-- Store Vector A
Commit(write_offset(VecA, 0, 3.0))
Commit(write_offset(VecA, 8, 4.0))
Commit(write_offset(VecA, 16, 5.0))

-- Store Vector B
Commit(write_offset(VecB, 0, 2.0))
Commit(write_offset(VecB, 8, -1.0))
Commit(write_offset(VecB, 16, 4.0))

Map(Dot3D, ignored,
    plus(
        multiply(read_offset(VecA, 0), read_offset(VecB, 0)),
        plus(
            multiply(read_offset(VecA, 8), read_offset(VecB, 8)),
            multiply(read_offset(VecA, 16), read_offset(VecB, 16))
        )
    )
)

Commit(Dot3D(0))
`
  },
  pid_controller: {
    name: "Closed-Loop PID Controller",
    tag: "Control Systems",
    description: "Proportional-integral-derivative control step simulation with state preserved across cycles.",
    code: `-- --------------------------------------------------
-- LLFPL: Discrete PID Controller Step
-- --------------------------------------------------
Require(stdlib.LLFPL)

Slab(PIDState, 64)
Identity(Kp, 1.25)
Identity(Ki, 0.05)
Identity(Kd, 0.15)
Identity(Setpoint, 50.0)

Identity(OFFSET_INT, 0)
Identity(OFFSET_PREV, 8)

Map(InitPID, val,
    plus(
        write_offset(PIDState, OFFSET_INT, 0.0),
        write_offset(PIDState, OFFSET_PREV, 0.0)
    )
)

Map(ComputeControl, current,
    Identity(Err, minus(Setpoint, current)),
    write_offset(PIDState, OFFSET_INT, plus(read_offset(PIDState, OFFSET_INT), Err)),
    plus(
        multiply(Kp, Err),
        plus(
            multiply(Ki, read_offset(PIDState, OFFSET_INT)),
            multiply(Kd, minus(Err, read_offset(PIDState, OFFSET_PREV)))
        )
    )
)

Commit(InitPID(0))
Commit(ComputeControl(20.0))
Commit(ComputeControl(35.0))
Commit(ComputeControl(48.5))
`
  }
};

