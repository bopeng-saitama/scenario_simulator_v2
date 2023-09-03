import os
import yaml
import subprocess

# Get the list of modified or created files
result = subprocess.run(["git", "diff", "--name-only", "HEAD^"], stdout=subprocess.PIPE)
modified_files = result.stdout.decode().split("\n")

# Create a new test configuration file
new_workflow = {
  "Scenario": []
}

# Add modified or created scenarios to the new test configuration file
for file in modified_files:
  if file.startswith("test_runner/scenario_test_runner/scenario/") and file.endswith(".yaml"):
    scenario_path = "$(find-pkg-share scenario_test_runner)/scenario/" + os.path.basename(file)
    new_workflow["scenarios"].append({"path": scenario_path})

# Write the new test configuration file
new_workflow_file = os.path.join(os.environ['GITHUB_WORKSPACE'], 'test_runner', 'scenario_test_runner', 'config', 'new_workflow.yaml')

with open(new_workflow_file, 'w') as f:
    yaml.dump(new_workflow, f)

