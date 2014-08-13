api = 2
core = 7.x

; Add migrate suite

; Migrate
projects[migrate][version] = 2.x-dev
projects[migrate][subdir] = contrib
projects[migrate][download][type] = git
projects[migrate][download][revision] = 0010411
projects[migrate][download][branch] = 7.x-2.x

; Drupal-to-Drupal data migration
projects[migrate_d2d][version] = 2.1-beta1
projects[migrate_d2d][subdir] = contrib
