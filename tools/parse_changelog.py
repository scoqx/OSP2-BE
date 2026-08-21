import re
import sys


def is_changelog_start(line: str) -> bool:
  return re.match(r"^\d\d\.\d\d\.\d\d\d\d be v\d+?\..+$", line) is not None


def parse_changelog(filename: str) -> list[str]:
  versions: list[str] = []
  change: list[str] = []

  with open(filename, "r") as changelog:
    lines = changelog.readlines()

    for line in lines:
      if is_changelog_start(line):
        if len(change) > 0:
          # Avoid uneccesary newlines at the end
          if len(change[-1].strip()) == 0:
            change = change[:-1]

          versions.append(''.join(change))
        
        change.clear()

      # First line is effectively empty, skip it
      if len(change) == 0 and len(line.strip()) == 0:
        continue

      change.append(line)

    if len(change) > 0:
      versions.append(''.join(change))

  return versions


def write_latest_version(filename: str, changelog: list[str]):
  with open(filename, "w") as file:
    version_line = changelog[-1].split("\n", 1)[0]
    version_name = version_line.split(" ")[2]

    file.write(version_name)


def write_latest_change(filename: str, changelog: list[str]):
  with open(filename, "w") as file:
    file.write(changelog[-1])


def write_reverse_changelog(filename: str, changelog: list[str]):
  with open(filename, "w") as file:
    file.write("\n".join(changelog[::-1]))


def main():
  if len(sys.argv) < 4:
    print("Not enough arguments. Expected arguments in the order:", file=sys.stderr)
    print("parse_changelog.py <full changelog> <version file> <latest change> [<reverse changelog>]", file=sys.stderr)
    sys.exit(1)

  full_changelog_filename = sys.argv[1]
  latest_version_filename = sys.argv[2]
  latest_change_filename = sys.argv[3]
  reverse_changelog_filename = sys.argv[4] if len(sys.argv) >= 5 else None

  changelog = parse_changelog(full_changelog_filename)

  if len(changelog) == 0:
    print("Parsed changelog returned 0 changes", file=sys.stderr)
    sys.exit(1)

  write_latest_version(latest_version_filename, changelog)
  write_latest_change(latest_change_filename, changelog)

  if reverse_changelog_filename is not None:
    write_reverse_changelog(reverse_changelog_filename, changelog)


if __name__ == '__main__':
  main()