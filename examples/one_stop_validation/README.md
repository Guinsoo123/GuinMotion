# One Stop Validation Example

This example runs the GuinMotion validation loop from C++:

1. Import `demo_robot.urdf`.
2. Import `targets.xml`.
3. Import `scene.xyz`.
4. Run `guinmotion.examples.target_demo_planner`.
5. Execute the trajectory through the simulation service.
6. Evaluate the trajectory against target tolerances.

Build with the normal project preset, then run:

```bash
./build/default/bin/guinmotion_one_stop_validation
```

Expected result:

```text
evaluation: PASS
```
