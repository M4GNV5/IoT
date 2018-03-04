thickness = 2;
hullSize = [60, 60, 35];

//screw hole radius
screwR = 1.5;
screwH = 3;
holderSize = 2 * screwR + 3;

//center of the screw holes relative to the first at (0, 0)
nodeMcuScrews = [
	[0, 0],
	[52, 0],
	[0, 25],
	[52, 25]
];
vs1053Screws = [
	[0, 0],
	[37, 0],
	[0, 37],
	[37, 37]
];

//y, z (actually 7.9, 2.5)
microUsbSize = [10, 4];

//y, z position of the usb port on the nodeMcu relative to the first screw
microUsbPos = [nodeMcuScrews[3][1] / 2 - microUsbSize[0] / 2, -1];

//y position of the center of the audio port on the vs1053 relative to the first screw
audioPortPos = 8.25;

//(outer!) audio port radius (inner diameter is obviousely 3.5)
audioPortR = 2.75;

module holders(screws, height, wallSpacing)
{
	for(curr = screws)
	{
		translate([curr[0], curr[1], 0]) difference()
		{
			translate([0, 0, height / 2])
				cube([holderSize, holderSize, height], center = true);

			translate([0, 0, height - screwH])
				cylinder(r = screwR, h = screwH + 1, $fn = 360);
		}
	}
};

difference()
{
	cube([hullSize[0] + 2 * thickness, hullSize[1] + 2 * thickness, hullSize[2] + thickness]);

	translate([thickness, thickness, thickness])
		cube([hullSize[0], hullSize[1], hullSize[2] + 1]);

	translate([
			-1,
			thickness + hullSize[1] / 2 - nodeMcuScrews[3][1] / 2 + microUsbPos[0],
			thickness + 10 - microUsbSize[1] + microUsbPos[1]
		])
		cube([thickness + 2, microUsbSize[0], microUsbSize[1]]);

	translate([
			-1,
			thickness + hullSize[1] / 2 - vs1053Screws[3][1] / 2 + audioPortPos,
			thickness + 30 - audioPortR
		])
		rotate([0, 90, 0])
		cylinder(r = audioPortR, h = thickness + 2, $fn = 360);
};

translate([thickness + holderSize / 2, hullSize[1] / 2 - nodeMcuScrews[3][1] / 2 + thickness, thickness])
	holders(nodeMcuScrews, 10, 0);

translate([thickness + holderSize / 2, hullSize[1] / 2 - vs1053Screws[3][1] / 2 + thickness, thickness])
{
	translate([1, 0, 0])
		holders(vs1053Screws, 30, 1);

	translate([-holderSize / 2, -holderSize / 2, 0])
		cube([1, holderSize, 30]);

	translate([-holderSize / 2, vs1053Screws[1][0] - holderSize / 2, 0])
		cube([1, holderSize, 30]);
}
