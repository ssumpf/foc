# ARCH:       BUILD_ARCH_x86 x86
# ARCHDESCR:  X86-32 architecture
# ARCHSELECT: ARCH_ENABLE_STACK_PROTECTOR

# ARCH:       BUILD_ARCH_amd64 amd64
# ARCHDESCR:  AMD64 architecture
# ARCHSELECT: ARCH_ENABLE_STACK_PROTECTOR

choice
	prompt "CPU variant"
	
config CPU_X86_586
	bool "586 type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_PENTIUM
	bool "Pentium type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_PENTIUMMMX
	bool "Pentium with MMX type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_PENTIUMPRO
	bool "Pentium Pro type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_686
	bool "686 type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_PENTIUM2
	bool "Pentium2 type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_PENTIUM3
	bool "Pentium3 type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_PENTIUMM
	bool "Pentium-M type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_PENTIUM4
	bool "Pentium4 type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_PRESCOTT
	bool "Pentium4 Prescott type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_NOCONA
	bool "Pentium Nocona type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_CORE2
	bool "Core2 type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_K6
	bool "K6 type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_K7
	bool "K7 type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_ATHLON4
	bool "Athlon type CPU"
	depends on BUILD_ARCH_x86

config CPU_X86_K8
	bool "K8 type CPU"
	depends on BUILD_ARCH_x86 || BUILD_ARCH_amd64

config CPU_X86_K10
	bool "K10/Barcelona type CPU"
	depends on BUILD_ARCH_x86 || BUILD_ARCH_amd64

config CPU_X86_OPTERON
	bool "Opteron type CPU"
	depends on BUILD_ARCH_x86 || BUILD_ARCH_amd64



endchoice


config CPU
	string
	default "586"        if CPU_X86_586
	default "pentium"    if CPU_X86_PENTIUM
	default "pentiummmx" if CPU_X86_PENTIUMMMX
	default "pentiumpro" if CPU_X86_PENTIUMPRO
	default "686"        if CPU_X86_686
	default "pentium2"   if CPU_X86_PENTIUM2
	default "pentium3"   if CPU_X86_PENTIUM3
	default "pentiumm"   if CPU_X86_PENTIUMM
	default "pentium4"   if CPU_X86_PENTIUM4
	default "prescott"   if CPU_X86_PRESCOTT
	default "core2"      if CPU_X86_CORE2
	default "nocona"     if CPU_X86_NOCONA
	default "K6"         if CPU_X86_K6
	default "K7"         if CPU_X86_K7
	default "athlon4"    if CPU_X86_ATHLON4
	default "K8"         if CPU_X86_K8
	default "amdfam10"   if CPU_X86_K10
	default "opteron"    if CPU_X86_OPTERON


