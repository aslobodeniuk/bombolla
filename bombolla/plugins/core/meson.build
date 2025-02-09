commands = [
  'create',
  'dump',
  'on',
  'set',
  'call',
  'bind',
  'dna'
]

foreach com : commands
shared_library(['lba-command-' + com],
               ['lba-command-' + com + '.c'],
               dependencies: [bombolla_core_dep],
              )

# TODO: test for each command
endforeach
